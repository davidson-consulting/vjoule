#include "top.hh"

#include <sys/time.h>
#include <sys/inotify.h>
#include <exiting.hh>

using namespace common;
using namespace ftxui;

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

namespace tools::vjoule {

    double max (double x, double y) {
		return x > y ? x : y;
    }
    
    Top::Top (const CommandLine & cmd) :
		_cmd (cmd),
		_cfgPath (),
		_config (),
		_glob ({})
    {
		this-> configure ();
    }

    /**
     * ===================================================================================
     * ===================================================================================
     * ================================   CONFIGURATION     ==============================
     * ===================================================================================
     * ===================================================================================
     */    

    void Top::configure () {
		this-> _start = this-> _api.get_machine_current_consumption_no_force ();

		this-> _cfgPath = utils::join_path (VJOULE_DIR, "config.toml");
		auto configPath = utils::get_absolute_path_if_exists (this-> _cfgPath);
		if (configPath == "") {
			LOG_DEBUG ("No config file found at : ", this-> _cfgPath);
			return;
		}
	
		this-> _config = utils::parse_file (configPath);
		this-> listPlugins ();
	
		auto sensor = this-> _config.getOr <utils::config::dict> ("sensor", {});
		this-> _freq = sensor.getOr <int> ("freq", 1);
		this-> _summary = this-> createSummary ();

		if (this-> _cmd.output != "") {
			this-> _output = fopen (this-> _cmd.output.c_str (), "w");
			if (this-> _output == nullptr) {
				std::cerr << "error: failed to open output file." << std::endl;
				throw TopError ();
			}
	    
			fprintf (this-> _output, "%17s ; %8s ; %8s ; %8s ; %8s\n", "TIMESTAMP", "PDU", "CPU", "RAM", "GPU");
			fflush (this-> _output);
		}
    }

    void Top::listPlugins () {
		for (auto it : this-> _config.keys ()) {
			if (it != "sensor") {
				auto dict = this-> _config.get<utils::config::dict> (it);
				auto name = dict.getOr <std::string> ("name", "");
				if (name != "") {
					auto jt = this-> _plugins.find (name);
					if (jt != this-> _plugins.end ()) {
						jt-> second.push_back (it.substr (0, it.find (":")));
					} else {
						std::vector <std::string> v;
						v.push_back (it.substr (0, it.find (":")));
						this-> _plugins.emplace (name, v);
					}
				}
			}
		}
    }

    void Top::dispose () {
		if (this-> _output != nullptr) {
			fclose (this-> _output);
		}
    }
    
    /**
     * ===================================================================================
     * ===================================================================================
     * ===================================   RUN LOOP     ================================
     * ===================================================================================
     * ===================================================================================
     */    

    
    void Top::run () {
		concurrency::timer t;
		if (this-> _cmd.output == "") {
			concurrency::spawn (this, &Top::asyncDisplay);
		} else this-> _isReady = true;

		this-> waitServiceIteration ();
		this-> _isRunning = true;
		while (this-> _isRunning) {
			this-> fetch ();
			this-> display ();
			this-> waitServiceIteration ();
		}
	
		this-> dispose ();
    }   
    
    void Top::fetch () {
		auto t = this-> _timer.time_since_start ();
		this-> _timer.reset ();
		this-> readConsumption (t);
    }

    void Top::readConsumption (float time) {
		auto f = this-> _api.get_machine_current_consumption_no_force ();
		auto diff = f - this-> _start;

		this-> _glob.cpuW = diff.cpu / time;
		this-> _glob.ramW = diff.ram / time;
		this-> _glob.gpuW = diff.gpu / time;
		this-> _glob.pduW = f.pdu_watts;
	    
		this-> _glob.cpuJ = f.cpu;
		this-> _glob.ramJ = f.ram;
		this-> _glob.gpuJ = f.gpu;
		this-> _glob.pduJ = f.pdu;

		this-> insertHistory ();
		this-> _start = f;
    }

    double Top::readConsumption (const std::string & path, const std::string & type) const {
		auto componentPath = utils::join_path (path, type);
		double value = 0.0;
		if (utils::file_exists (componentPath)) {
			std::ifstream f (componentPath);
			f >> value;
			f.close ();
		}

		return value;
    }

    void Top::insertHistory () {
		this-> _cpuHist.push_back (this-> _glob.cpuW);
		this-> _ramHist.push_back (this-> _glob.ramW);
		this-> _gpuHist.push_back (this-> _glob.gpuW);
		this-> _pduHist.push_back (this-> _glob.pduW);
    }
    
    void Top::waitServiceIteration () const {
		std::string resultPath = utils::join_path (VJOULE_DIR, "results/cpu");
		auto fd = inotify_init ();
		auto wd = inotify_add_watch (fd, utils::join_path (VJOULE_DIR, "results").c_str (), IN_MODIFY);
    
		char buffer[EVENT_BUF_LEN];
		while (true) {
			auto len = read (fd, buffer, EVENT_BUF_LEN);
			if (len == 0) {
				throw std::runtime_error ("waiting inotify");
			}

			int i = 0;
			while (i < len) {
				struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
				if (event-> len != 0 && event-> mask & IN_MODIFY && strcmp (event-> name, "cpu") == 0) {
					inotify_rm_watch (fd, wd);
					close (fd);
					return;
				}
	    
				i += EVENT_SIZE + event->len;
			}
		}
    }        
    
    /**
     * ===================================================================================
     * ===================================================================================
     * ===================================   RENDERING     ===============================
     * ===================================================================================
     * ===================================================================================
     */    

    void Top::display () {
		if (this-> _cmd.output == "") {
			this-> _content = this-> createTable ();
			if (this-> _isRunning) {
				concurrency::timer t;
				while (!this-> _isReady) {
					t.sleep (0.05);
				}
		
				this-> _screen.PostEvent (Event::Custom);
			}
		} else {
			this-> exportCsv ();
		}
    }

    void Top::asyncDisplay (concurrency::thread) {
		auto sumR = Renderer ([&](bool) { return this-> _summary; });
		auto tableR = Renderer ([&](bool) {
			auto tab = this-> createTable ();
			auto graph = this-> createGraph ();
			return window (text(L" Consumption "), vbox ({tab, graph}) | flex) | flex;
		});

		this-> _isReady = true;
		this-> _screen.Loop (Container::Vertical ({sumR, tableR}));
		this-> _isRunning = false;
    }
    
    Element Top::createSummary () const {	              
		std::vector <Element> childrens;
		childrens.push_back (text (L"frequency :   "));
		childrens.push_back (text (std::to_string (this-> _freq)) | bold);
		childrens.push_back (separator ());
		childrens.push_back (separator ());
		for (auto & it : this-> _plugins) {
			std::stringstream inner;
			inner << it.first << "@{";
			int i = 0;
			for (auto & p : it.second) {
				if (i != 0) inner << ", ";
				inner << p;
				i += 1;
			}
			inner << "}";
			childrens.push_back (text (L"Plugin : "));
			childrens.push_back (text (inner.str ()));
			childrens.push_back (separator ());
		}
			
		auto content = vbox({hbox (childrens)});
		return window(text(L" Summary "), content);
    }

    void Top::exportCsv () {
		static int i = 0;
		struct timeval start;
		gettimeofday(&start, NULL);
		fprintf (this-> _output, "%ld.%ld ; %8.2lf ; %8.2lf ; %8.2lf ; %8.2lf\n", start.tv_sec, start.tv_usec, this-> _glob.pduJ, this-> _glob.cpuJ, this-> _glob.ramJ, this-> _glob.gpuJ);
		fflush (this-> _output);
	
		std::cout << ".";
		std::cout.flush ();
		i += 1;
		if (i > 20) {
			std::cout << std::endl;
			i = 0;
		}
    }

    Element Top::createTable () const {
		std::vector <std::vector <std::string> > rows;
		std::vector <std::string> head {"PDU", "CPU", "RAM", "GPU"};
		rows.push_back (head);

		char buf [255];
		if (this-> _glob.cpuJ >= 10000) {
			snprintf (buf, 255, "%.2lf W / %.2lf kJ", this-> _glob.cpuW, this-> _glob.cpuJ / 1000);
		} else {
			snprintf (buf, 255, "%.2lf W / %.2lf J", this-> _glob.cpuW, this-> _glob.cpuJ);
		}

		std::string cpu (buf);

		if (this-> _glob.ramJ >= 10000) {
			snprintf (buf, 255, "%.2lf W / %.2lf kJ", this-> _glob.ramW, this-> _glob.ramJ / 1000);
		} else {
			snprintf (buf, 255, "%.2lf W / %.2lf J", this-> _glob.ramW, this-> _glob.ramJ);
		}
		std::string ram (buf);

		if (this-> _glob.pduJ > 10000) {
			snprintf (buf, 255, "%.2lf W / %.2lf kJ", this-> _glob.pduW, this-> _glob.pduJ / 1000);
		} else {
			snprintf (buf, 255, "%.2lf W / %.2lf J", this-> _glob.pduW, this-> _glob.pduJ);
		}
		std::string pdu (buf);

		if (this-> _glob.gpuJ > 10000) {
			snprintf (buf, 255, "%.2lf W / %.2lf kJ", this-> _glob.gpuW, this-> _glob.gpuJ / 1000);
		} else {
			snprintf (buf, 255, "%.2lf W / %.2lf J", this-> _glob.gpuW, this-> _glob.gpuJ);
		}
		std::string gpu (buf);

		std::vector <std::string> row {pdu, cpu, ram, gpu};
		rows.push_back (row);

		auto tab = Table (rows);
		tab.SelectAll ().Border (LIGHT);
		tab.SelectColumns (0, 3).DecorateCells (size (WIDTH, GREATER_THAN, 40));
		tab.SelectRow (0).Border (DOUBLE);
		tab.SelectRow (0).Separator (LIGHT);


		return std::move (tab).Render ();
    }

    Element Top::createGraph () {
		auto pduGraph = graph ([&](int w,int h) {
			std::vector <int> o (w);
			if (this-> _pduHist.size () > w) {
				auto i = this-> _pduHist.size () - w;
				this-> _pduHist = std::vector <double> (this-> _pduHist.begin () + i, this-> _pduHist.end ());
			}

			auto scale = (double) h / this-> findMax (this-> _pduHist);
			for (int i = 0; i < this-> _pduHist.size () ; i++) {
				o[i] = this-> _pduHist[i] * scale;
			}
			return o;
		}) | color (Color::GreenLight);

		auto cpuGraph = graph ([&](int w,int h) {
			std::vector <int> o (w);
			if (this-> _cpuHist.size () > w) {
				auto i = this-> _cpuHist.size () - w;
				this-> _cpuHist = std::vector <double> (this-> _cpuHist.begin () + i, this-> _cpuHist.end ());
			}
		    
			auto scale = (double) h / this-> findMax (this-> _cpuHist);
			for (int i = 0 ; i < this-> _cpuHist.size () ; i++) {
				o[i] = this-> _cpuHist[i] * scale;
			}
	    
			return o;
		}) | color (Color::BlueLight);

		auto ramGraph = graph ([&](int w,int h) {
			std::vector <int> o (w);
			if (this-> _ramHist.size () > w) {
				auto i = this-> _ramHist.size () - w;
				this-> _ramHist = std::vector <double> (this-> _ramHist.begin () + i, this-> _ramHist.end ());
			}

			auto scale = (double) h / this-> findMax (this-> _ramHist);
			for (int i = 0; i < this-> _ramHist.size () ; i++) {
				o[i] = this-> _ramHist[i] * scale;
			}
	    
			return o;
		}) | color (Color::RedLight);

		auto gpuGraph = graph ([&](int w,int h) {
			std::vector <int> o (w);
			if (this-> _gpuHist.size () > w) {
				auto i = this-> _gpuHist.size () - w;
				this-> _gpuHist = std::vector <double> (this-> _gpuHist.begin () + i, this-> _gpuHist.end ());
			}

			auto scale = (double) h / this-> findMax (this-> _gpuHist);
			for (int i = 0; i < this-> _gpuHist.size () ; i++) {
				o[i] = this-> _gpuHist[i] * scale;
			}
			return o;
		}) | color (Color::YellowLight);


		pduGraph = vbox({text (std::to_string (this-> findMax (this-> _pduHist))), pduGraph});
		cpuGraph = vbox({text (std::to_string (this-> findMax (this-> _cpuHist))), cpuGraph});
		gpuGraph = vbox({text (std::to_string (this-> findMax (this-> _gpuHist))), gpuGraph});
		ramGraph = vbox({text (std::to_string (this-> findMax (this-> _ramHist))), ramGraph});

		return vbox ({
				window (text (" PDU "), pduGraph | flex) | flex,
				window (text (" CPU "), cpuGraph | flex) | flex,
				window (text (" RAM "), ramGraph | flex) | flex,
				window (text (" GPU "), gpuGraph | flex) | flex
			}) | flex;
    }


    double Top::findMax (const std::vector <double> & v) const {
		double r = 1.0;
		for (auto & i : v) {
			if (i > r) r = i;
		}
		return r;
    }
    
}
