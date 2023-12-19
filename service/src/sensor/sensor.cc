#include <sensor/sensor.hh>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/inotify.h>

using namespace common;


#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

namespace sensor {

    common::concurrency::signal <> exitSignal;
    
    Sensor::Sensor () {}

    void Sensor::configure (int argc, char ** argv) {
		this-> _argc = argc;
		this-> _argv = argv;
    
		this-> initAppOptions ();
		if (this-> _pluginHOpt-> count () > 0) {
			this-> displayPluginHelp ();
			exit (0);
		}
	
		if (this-> _cfgPathOpt-> count () > 0) {
			this-> _cfgPath = this-> _cfgPathOpt-> as<std::string> ();
		} else this-> _cfgPath = utils::join_path (VJOULE_DIR, "config.toml");
	
		auto configPath = utils::get_absolute_path_if_exists (this-> _cfgPath);
		if (configPath == "") {
			LOG_ERROR ("No config file found at : ", this-> _cfgPath);
			exit (0);
		}
	
		this-> configure (utils::parse_file (configPath));
    }

    void Sensor::runAsync () {
		LOG_DEBUG ("Starting sensor async");
		this-> _th = concurrency::spawn (this, &Sensor::mainLoop);
		if (this-> _freq != 0) {
			this-> _ph = concurrency::spawn (this, &Sensor::pingNotification);
		}
    }
    
    void Sensor::run () {	
		LOG_DEBUG ("Starting main loop");
		if (this-> _freq != 0) {
			this-> _ph = concurrency::spawn (this, &Sensor::pingNotification);
		}
		this-> mainLoop (0);
    }

    void Sensor::forcedIteration () {
		this-> _mt.lock ();
		this-> _computeCore ();
		this-> _mt.unlock ();
    }
    
    void Sensor::mainLoop (concurrency::thread) {
		concurrency::timer timer;
		this-> _isRunning = true;
		while (this-> _isRunning) {
			this-> waitSignal ();
	    
			timer.reset ();
			this-> forcedIteration ();

			auto took = timer.time_since_start ();
			LOG_DEBUG ("Main loop iteration : ", took, "s");
		}
    }

    void Sensor::configureSignal () {
		this-> _inotifFd = inotify_init ();
		this-> _inotifFdW = inotify_add_watch (this-> _inotifFd, this-> _signalPath.c_str (), IN_MODIFY | IN_CREATE);
    }
    
    void Sensor::waitSignal () {
		char buffer[EVENT_BUF_LEN];
		while (true) {
			auto len = read (this-> _inotifFd, buffer, EVENT_BUF_LEN);
			if (len == 0) {
				throw std::runtime_error ("reading cgroup notif");
			}

			int i = 0;
			while (i < len) {
				struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
				if (event-> len != 0) { // there is an event to read
					if (event-> mask & IN_CREATE || event-> mask & IN_DELETE | event-> mask & IN_MODIFY) {
						if (this-> _signalName == event-> name) return;
					}
				}
				i += EVENT_SIZE + event->len;
			}
		}
    }    

    void Sensor::pingNotification (concurrency::thread) {
		concurrency::timer timer;
		while (this-> _isRunning) {
			timer.reset ();

			this-> _mt.lock ();
			fseek (this-> _signalFD, 0, SEEK_SET);
			int i = 1;
			fwrite (&i, sizeof (int), 1, this-> _signalFD);
			fflush (this-> _signalFD);
			this-> _mt.unlock ();

			auto took = timer.time_since_start ();
			float toSleep = this-> _freq - took - 0.001;
			if (toSleep > 0.0f) {
				timer.sleep (toSleep);
			}
	    	    
		}
    }
    
    void Sensor::dispose () {
		LOG_INFO ("Disposing service.");

		this-> _isRunning = false;
		if (this-> _signalFD != nullptr) {
			this-> _mt.lock ();
			fseek (this-> _signalFD, 0, SEEK_SET);
			int i = 1;
			fwrite (&i, sizeof (int), 1, this-> _signalFD);
			fflush (this-> _signalFD);
			this-> _mt.unlock ();
		}

		if (this-> _th != 0) {
			concurrency::join (this-> _th);
			this-> _th = 0;
		}

		if (this-> _ph != 0) {
			concurrency::join (this-> _ph);
			this-> _ph = 0;
		}

		LOG_INFO ("Finish waiting.");

		if (this-> _inotifFdW != 0) {
			inotify_rm_watch (this-> _inotifFd, this-> _inotifFdW);
			close (this-> _inotifFd);
			this-> _inotifFd = 0;
			this-> _inotifFdW = 0;
		}

		LOG_INFO ("Closed inotify.");

		if (this-> _signalFD != nullptr) {
			fclose (this-> _signalFD);
			this-> _signalFD = nullptr;
		}

		LOG_INFO ("Closed signal file.");

		this-> _factory.dispose ();
		LOG_INFO ("Disposed plugins.");

		this-> _core-> dispose ();
		LOG_INFO ("Disposed core plugin.");

		utils::Logger::clear ();
	
		pfm_terminate ();
		LOG_INFO ("Terminated.");
    }    
    
    /**
     * ===================================================================================
     * ===================================================================================
     * ================================   CONFIGURATION     ==============================
     * ===================================================================================
     * ===================================================================================
     */    
    
    void Sensor::initAppOptions () {
		std::string configPath, logPath, logLevel;
		std::string pluginH;
		int port;
		float freq;
		bool h;
	
		this-> _cfgPathOpt = this-> _app.add_option ("-c,--config-path", configPath, "the path of the configuration file");
		this-> _logPathOpt = this-> _app.add_option ("-l,--log-path", logPath, "the path of the log file");
		this-> _logLvlOpt = this-> _app.add_option ("-v,--log-lvl", logLevel, "the lvl of the log");
		this-> _freqOpt = this-> _app.add_option ("-f,--freq", freq, "the frequency of the sensor");
		this-> _pluginHOpt = this-> _app.add_option ("--ph,--plugin-help", pluginH, "display the help of a plugin, takes a plugin name as parameter");

	
		try {
			this-> _app.parse (this-> _argc, this-> _argv);
		} catch (const CLI::ParseError &e) {
			exit (this-> _app.exit (e));
		}
    }
    
    void Sensor::configure (const common::utils::config::dict & config) {	
		auto sensorConfig = config.getOr <utils::config::dict> ("sensor", {});

		auto fr = sensorConfig.getOr <float> ("freq", 1.0f);
		if (fr == 0) {
			this-> _freq = 0;
		} else {
			this-> _freq = 1.0f / fr;
		}
		auto newLogFile = sensorConfig.getOr <std::string> ("log-path", "");
		if (utils::parent_directory (newLogFile) == "" || utils::parent_directory (newLogFile) == "/") {
			LOG_ERROR ("Cannot put log file in root directory.");
			throw std::runtime_error ("service.");
		}
	
		utils::Logger::globalInstance ().redirect (newLogFile, true);
		utils::Logger::globalInstance ().changeLevel (sensorConfig.getOr<std::string> ("log-lvl", "SUCCESS"));

		auto signalFile = sensorConfig.getOr <std::string> ("signal-path", utils::join_path (VJOULE_DIR, "signal"));
		if (utils::parent_directory (signalFile) == "" || utils::parent_directory (signalFile) == "/") {
			LOG_ERROR ("Cannot put signal file in root directory.");
			throw std::runtime_error ("service.");
		}
	
		this-> _signalFD = fopen (signalFile.c_str (), "w");
		utils::own_file (signalFile, "vjoule");

		if (this-> _signalFD == nullptr) {
			LOG_ERROR ("Failed to open ", signalFile, " permission denied");
			throw std::runtime_error ("service.");
		}

		this-> _signalPath = utils::parent_directory (signalFile);
		this-> _signalName = signalFile.substr (this-> _signalPath.length () + 1);
	
		this-> configureSignal ();
		this-> configureOptions ();

		if (this-> _freq == 0) {
			LOG_INFO ("Sensor started in synchronus mode.");
		}

		for (auto it : config.keys ()) {
			if (it != "sensor") {
				auto dict = config.get<utils::config::dict> (it);
				this-> configurePlugin (it, dict);
			}
		}

		if (sensorConfig.has <std::string> ("core")) {
			this-> configureCore (sensorConfig);
		} else {
			LOG_ERROR ("Core is not defined in the configuration.");
			throw std::runtime_error ("service.");
		}

		cgroup::Cgroup c ("vjoule_api.slice");
		c.create ();

		exitSignal.connect (this, &Sensor::dispose);
    }

    void Sensor::configureOptions () {
		if (this-> _logPathOpt-> count () > 0) {
			auto newLogFile = this-> _logPathOpt-> as<std::string> ();
			utils::Logger::globalInstance ().redirect (newLogFile, false);
		}

		if (this-> _logLvlOpt-> count () > 0) {
			utils::Logger::globalInstance ().changeLevel (this-> _logLvlOpt-> as<std::string> ());
		}

		if (this-> _freqOpt-> count () > 0) {
			auto fr = this-> _freqOpt-> as<float> ();
			if (fr == 0) {
				this-> _freq = 0;
			} else {
				this-> _freq=  1.0 / fr;
			}
		}
    }   

    void Sensor::configureCore (const common::utils::config::dict & config) {
		pfm_initialize ();
	
		this-> _core = new common::plugin::Plugin ("core", config.get<std::string> ("core"));
		this-> _core-> configure ();
		auto initFunc = this-> _core-> getFunction <common::plugin::CoreInitFunc_t> ("init");
		auto success = initFunc (&config, &this-> _factory);
	
		if (success) {

			this-> _computeCore = this-> _core-> getFunction <common::plugin::CoreComputeFunc_t> ("compute");
			if (this-> _computeCore == nullptr) {
				LOG_ERROR ("Core plugin has no 'void compute ()' function");
				throw std::runtime_error ("service.");
			}
			return;
		} else {
			delete this-> _core;
			this-> _core = nullptr;
		}
	
		LOG_ERROR ("Configuration of core failed.");
		throw std::runtime_error ("service.");
    }

    void Sensor::configurePlugin (const std::string & kind, const common::utils::config::dict & config) {
		auto k = kind;
		if (k.find (":") != std::string::npos) {
			k = kind.substr (0, k.find (":"));
		}
	
		if (!this-> _factory.configurePlugin (k, config)) {
			LOG_ERROR ("Failed to configure plugin of kind '", k, "' : ", config);
			throw std::runtime_error ("service.");
		}
    }


    /**
     * ===================================================================================
     * ===================================================================================
     * ================================        HELP         ==============================
     * ===================================================================================
     * ===================================================================================
     */    


    void Sensor::displayPluginHelp () {
		auto pluginName = this-> _pluginHOpt-> as<std::string> ();
		auto plugin = new common::plugin::Plugin ("", pluginName);
		if (!plugin-> configure ()) {
			std::cerr << "No plugin named '" << pluginName << "' found" << std::endl;
			delete plugin;
			return;
		}
	
		auto helpFunc = plugin-> getFunction <common::plugin::HelpFunc_t> ("help");
		if (helpFunc == nullptr) {
			std::cerr << "Plugin '" << pluginName << "' found, but has no help function." << std::endl;
			delete plugin;
			return;
		}

		std::cout << helpFunc () ;

		plugin-> dispose ();
		delete plugin;
    }        

}
