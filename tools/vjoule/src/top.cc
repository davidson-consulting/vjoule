#include "top.hh"

#include <filesystem>
#include <sys/inotify.h>


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
	auto check = concurrency::SubProcess ("systemctl", {"is-active","--quiet", "vjoule_service.service"}, ".");
	check.start ();

	if (check.wait () != 0) {
	    std::cout << "vJoule service is not running." << std::endl;
	    exit (-1);
	}

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

	this-> createAndRegisterCgroup ();	
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

    void Top::createAndRegisterCgroup () {
	cgroup::Cgroup c ("vjoule.slice/top");
	c.create ();
	
	if (!c.attach (getpid ())) {
	    std::cerr << "vJoule failed to create cgroup" << std::endl;
	    exit (-1);
	}
    }

    void Top::dispose () {
	cgroup::Cgroup c ("vjoule.slice/top");
	c.detach (getpid ());
	c.remove ();	
    }
    
    /**
     * ===================================================================================
     * ===================================================================================
     * ===================================   RUN LOOP     ================================
     * ===================================================================================
     * ===================================================================================
     */    

    
    void Top::run () {	
	concurrency::spawn (this, &Top::asyncDisplay);
	concurrency::timer t;
	t.sleep (0.2);

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
	
	std::map <std::string, Result> res;	
	this-> readCgroupTree (res, utils::join_path (VJOULE_DIR, "results"), t);

	this-> _results = std::move (res);
    }

    void Top::readCgroupTree (std::map <std::string, Result> & res, const std::string & currentPath, float time) {
	if (utils::file_exists (utils::join_path (currentPath, "cpu"))) {
	    this-> readCgroupConsumption (res, currentPath, time);
	}

	for (const auto & entry : std::filesystem::directory_iterator (currentPath)) {
	    if (std::filesystem::is_directory (entry.path ())) {
		this-> readCgroupTree (res, entry.path (), time);
	    }
	}
    }

    void Top::readCgroupConsumption (std::map <std::string, Result> & res, const std::string & cgroupPath, float time) {
	auto cpu = this-> readConsumption (cgroupPath, "cpu");
	auto ram = this-> readConsumption (cgroupPath, "ram");
	auto gpu = this-> readConsumption (cgroupPath, "gpu");
	auto cgroupName = cgroupPath.substr (utils::join_path (VJOULE_DIR, "results").length ());
	
	if (cgroupName == "") {
	    this-> _glob.cpuP = 100;
	    this-> _glob.ramP = 100;
	    this-> _glob.gpuP = 100;

	    if (this-> _glob.cpuJ != 0) {
		this-> _glob.cpuW = (cpu - this-> _glob.cpuJ) / time;
		this-> _glob.ramW = (ram - this-> _glob.ramJ) / time;
		this-> _glob.gpuW = (gpu - this-> _glob.gpuJ) / time;
	    }
	    
	    this-> _glob.cpuJ = cpu;
	    this-> _glob.ramJ = ram;
	    this-> _glob.gpuJ = gpu;

	    res.emplace ("#SYSTEM", this-> _glob);
	    this-> insertHistory ();
	} else {
	    auto it = this-> _results.find (cgroupName);
	    Result r {
		.cpuJ = cpu, .ramJ = ram, .gpuJ = gpu,
		.cpuW = 0, .ramW = 0, .gpuW = 0,
		.cpuP = cpu / max (1, this-> _glob.cpuJ) * 100,
		.ramP = ram / max (1, this-> _glob.ramJ) * 100,
		.gpuP = gpu / max (1, this-> _glob.gpuJ) * 100
	    };
	    
	    if (it != this-> _results.end ()) {
		r.cpuW = (cpu - it-> second.cpuJ) / time;
		r.ramW = (ram - it-> second.ramJ) / time;
		r.gpuW = (gpu - it-> second.gpuJ) / time;
	    } 

	    res.emplace (cgroupName, r);	    	    
	}		
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
	this-> _content = this-> createTable ();
	if (this-> _isRunning) {
	    this-> _screen.PostEvent (Event::Custom);
	}
    }

    void Top::asyncDisplay (concurrency::thread) {
	auto sumR = Renderer ([&](bool) { return this-> _summary; });
	auto tableR = Renderer ([&](bool) {
	    auto tab = this-> createTable ();
	    auto graph = this-> createGraph ();
	    return window (text(L" Consumption "), vbox ({tab, graph}) | flex) | flex;
	});
	
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


    std::vector <std::pair <std::string, Result> > Top::sortByCpu () const {
	std::vector <std::pair <std::string, Result> > res;
	for (auto & it : this-> _results) {
	    res.push_back (it);
	}

	sort (res.begin (), res.end (), [](const std::pair <std::string, Result> & r1,
					   const std::pair <std::string, Result> & r2) {
	    return r1.second.cpuP > r2.second.cpuP;
	});

	return res;
    }
    
    Element Top::createTable () const {
	std::vector <std::vector <std::string> > rows;
	std::vector <std::string> head {"Cgroup", "CPU", "RAM", "GPU"};
	rows.push_back (head);

	auto r = this-> sortByCpu ();
	for (auto & it : r) {
	    char buf [255];
	    snprintf (buf, 255, "%.2lfW / %.2lfJ (%.1lf%c)", it.second.cpuW, it.second.cpuJ, it.second.cpuP, '%');
	    std::string cpu (buf);

	    snprintf (buf, 255, "%.2lfW / %.2lfJ (%.1lf%c)", it.second.ramW, it.second.ramJ, it.second.ramP, '%');
	    std::string ram (buf);

	    snprintf (buf, 255, "%.2lfW / %.2lfJ (%.1lf%c)", it.second.gpuW, it.second.gpuJ, it.second.gpuP, '%');
	    std::string gpu (buf);

	    
	    std::vector <std::string> row {it.first, cpu, ram, gpu};
	    rows.push_back (row);
	}
	
	auto tab = Table (rows);
	tab.SelectAll ().Border (LIGHT);
	tab.SelectColumn (0).DecorateCells (flex);
	tab.SelectColumns (1, 3).DecorateCells (size (WIDTH, GREATER_THAN, 40));
	tab.SelectRow (0).Border (DOUBLE);
	tab.SelectRow (0).Separator (LIGHT);
	tab.SelectRectangle (0, 0, 1, 1).DecorateCells (bold);

	return std::move (tab).Render ();
    }

    Element Top::createGraph () {
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

	
	cpuGraph = vbox({text (std::to_string (this-> findMax (this-> _cpuHist))), cpuGraph});
	gpuGraph = vbox({text (std::to_string (this-> findMax (this-> _gpuHist))), gpuGraph});
	ramGraph = vbox({text (std::to_string (this-> findMax (this-> _ramHist))), ramGraph});

	return vbox ({
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
