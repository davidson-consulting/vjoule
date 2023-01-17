#include <sensor/sensor.hh>
#include <filesystem>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>

using namespace common;

namespace sensor {


    Sensor::Sensor (int argc, char ** argv) :
	_argc (argc),
	_argv (argv)
    {
	this-> initAppOptions ();
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

    void Sensor::run () {
	LOG_DEBUG ("Starting main loop");
	concurrency::timer timer;
	while (true) {
	    timer.reset ();
	    this-> _computeCore ();

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
	int port;
	float freq;
	
	this-> _cfgPathOpt = this-> _app.add_option ("-c,--config-path", configPath, "the path of the configuration file");
	this-> _logPathOpt = this-> _app.add_option ("-l,--log-path", logPath, "the path of the log file");
	this-> _logLvlOpt = this-> _app.add_option ("-v,--log-lvl", logLevel, "the lvl of the log");
	this-> _freqOpt = this-> _app.add_option ("-f,--freq", freq, "the frequency of the sensor");
	
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
	    exit (-1);
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
		exit (-1);
	    }
	    return;	    
	} else {
	    delete this-> _core;
	    this-> _core = nullptr;
	}
	
	LOG_ERROR ("Configuration of core failed.");
	exit (-1);	
    }

    void Sensor::configurePlugin (const std::string & kind, const common::utils::config::dict & config) {
	if (!this-> _factory.configurePlugin (kind, config)) {
	    LOG_ERROR ("Failed to configure plugin of kind ", kind, " : ", config);
	    // exit (-1);
	}
    }
    
    

}