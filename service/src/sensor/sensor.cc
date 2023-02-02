#include <sensor/sensor.hh>
#include <filesystem>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>

using namespace common;

namespace sensor {

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
	    LOG_DEBUG ("No config file found at : ", this-> _cfgPath);
	    return;
	}
	
	this-> configure (utils::parse_file (configPath));
    }

    void Sensor::runAsync () {
	LOG_DEBUG ("Starting sensor async");
	this-> _th = concurrency::spawn (this, &Sensor::mainLoop);
    }
    
    void Sensor::run () {	
	LOG_DEBUG ("Starting main loop");
	this-> mainLoop (0);
    }

    void Sensor::stop () {
	this-> _isRunning = false;
	if (this-> _th != 0) {
	    concurrency::join (this-> _th);
	}
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
	    timer.reset ();
	    this-> _mt.lock ();
	    this-> _computeCore ();
	    this-> _mt.unlock ();

	    auto took = timer.time_since_start ();
	    float toSleep = this-> _freq - took - 0.001;
	    if (toSleep > 0.0f) {
		timer.sleep (toSleep);
	    }
	    
	    LOG_DEBUG ("Main loop iteration : ", took, "s");	    
	}
    }

    void Sensor::dispose () {
	this-> _factory.dispose ();
	this-> _core-> dispose ();
	utils::Logger::clear ();
	
	pfm_terminate ();	
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

	this-> _freq = 1.0f / sensorConfig.getOr <float> ("freq", 1.0f);

	auto newLogFile = sensorConfig.getOr <std::string> ("log-path", "");
	utils::Logger::globalInstance ().redirect (newLogFile, true);
	utils::Logger::globalInstance ().changeLevel (sensorConfig.getOr<std::string> ("log-lvl", "SUCCESS"));
	
	this-> configureOptions ();
	
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
	    throw 1;
	}	
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
	    this-> _freq = 1.0 / this-> _freqOpt-> as<float> ();
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
		throw 1;
	    }
	    return;	    
	} else {
	    delete this-> _core;
	    this-> _core = nullptr;
	}
	
	LOG_ERROR ("Configuration of core failed.");
	throw 1;	
    }

    void Sensor::configurePlugin (const std::string & kind, const common::utils::config::dict & config) {
	auto k = kind;
	if (k.find (":") != std::string::npos) {
	    k = kind.substr (0, k.find (":"));
	}
	
	if (!this-> _factory.configurePlugin (k, config)) {
	    LOG_ERROR ("Failed to configure plugin of kind '", k, "' : ", config);
	    throw 1;
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
