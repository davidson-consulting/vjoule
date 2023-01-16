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
	pfm_initialize ();
	this-> initAppOptions ();
	if (this-> _cfgPathOpt-> count () > 0) {
	    this-> _cfgPath = this-> _cfgPathOpt-> as<std::string> ();
	} else this-> _cfgPath = utils::join_path (VJOULE_DIR, "config.toml");

	if (this-> _cgroupPathOpt-> count () > 0) {
	    this-> _cgroupFile = this-> _cgroupPathOpt-> as <std::string> ();
	} else this-> _cgroupFile = utils::join_path (VJOULE_DIR, "cgroups");
	
	auto configPath = utils::get_absolute_path_if_exists (this-> _cfgPath);
	if (configPath == "") {
	    utils::Logger::globalInstance ().debug ("No config file found at : ", this-> _cfgPath);
	    return;
	}
	
	this-> configure (utils::parse_file (configPath));
    }

    void Sensor::run () {
	this-> _logger.debug ("Starting main loop");
	concurrency::timer timer;
	while (true) {
	    timer.reset ();
	    
	    this-> pollPlugins ();
	    this-> pollPerfEvents ();
	    if (this-> _formula != nullptr) {
		this-> callFormula ();		
	    }

	    if (this-> _dump) {
		this-> dumpPlugins ();
		this-> dumpFormula ();
	    }

	    auto took = timer.time_since_start ();
	    float toSleep = this-> _freq - took - 0.001;
	    if (toSleep > 0.0f) {
		timer.sleep (toSleep);
	    }	    
	    
	    this-> _logger.debug ("Main loop iteration : ", took, "s");	    
	}
    }

    void Sensor::dispose () {
	this-> _factory.dispose ();
	if (this-> _formula != nullptr) {
	    this-> _formula-> dispose ();
	    delete this-> _formula;
	    this-> _formula = nullptr;
	}
	
	pfm_terminate ();
    }


    /**
     * ===================================================================================
     * ===================================================================================
     * ====================================  CALLING     =================================
     * ===================================================================================
     * ===================================================================================
     */    


    void Sensor::pollPlugins () {
	for (auto & it : this-> _factory.getPlugins ()) {
	    it.second-> poll ();	    
	}	
    }

    void Sensor::pollPerfEvents () {
	for (uint64_t i = 0 ; i < this-> _cgroupWatchers.size () ;i++) {
	    this-> _cgroupWatchers[i].poll (this-> _perfEventValues[i]);
	}
    }

    void Sensor::callFormula () {
	this-> _formulaCompute (this-> _cgroupList,
				this-> _perfEventValues,
				this-> _factory);
    }

    void Sensor::dumpFormula () {
	if (this-> _formulaDump != nullptr) {
	    this-> _formulaDump (this-> _cgroupList,
				 this-> _perfEventValues,
				 this-> _factory);
	}
    }
    
    void Sensor::dumpPlugins () {	
	for (auto &it : this-> _factory.getPlugins ()) {
	    auto & out = this-> _dumpingFiles[it.first];
	    it.second-> dump (out);	    
	}
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
	this-> _cgroupPathOpt = this-> _app.add_option ("--cgroup-file", configPath, "the path of the configuration file");
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

	auto oldLogFile = this-> _logger.getLogFilePath ();
	auto newLogFile = sensorConfig.getOr <std::string> ("log-path", "");
	if (oldLogFile != newLogFile) {
	    this-> _logger.redirect (newLogFile, true);
	}
	this-> _logger.changeLevel (sensorConfig.getOr<std::string> ("log-lvl", "SUCCESS"));
	this-> _logger.setName ("sensor");
	
	this-> configureOptions ();
	
	std::string logLevel = this-> _logger.getLogLevel ();
	std::string logPath = this-> _logger.getLogFilePath ();
	if (config.has <utils::config::dict> ("formula")) {
	    auto dict = config.getOr <utils::config::dict> ("formula", {});
	    dict.insert ("log-lvl", new std::string (logLevel), typeid (std::string).name ());
	    dict.insert ("log-path", new std::string (logPath), typeid (std::string).name ());
	    
	    this-> configureFormula (dict);
	} else {
	    this-> _logger.info ("Configuring sensor without formula.");
	}
	
	for (auto it : config.keys ()) {
	    if (it != "sensor" && it != "formula") {
		auto dict = config.get<utils::config::dict> (it);
		dict.insert ("log-lvl", new std::string (logLevel), typeid (std::string).name ());
		dict.insert ("log-path", new std::string (logPath), typeid (std::string).name ());
		
		this-> configurePlugin (it, dict);
	    }
	}

	auto nbMaxOpen = sensorConfig.getOr <int> ("max-open-files", 65535);
	struct rlimit r;
	r.rlim_cur = nbMaxOpen;
	r.rlim_max = nbMaxOpen;

	this-> _logger.info ("Setting max open files : ", nbMaxOpen);
	setrlimit (RLIMIT_NOFILE, &r);
	getrlimit (RLIMIT_NOFILE, &r);

	if (r.rlim_cur != nbMaxOpen) {
	    this-> _logger.error ("Failed setting max open files, current max is : ", r.rlim_cur);
	}
	
	this-> configurePerfEventWatchers ();
    }

    void Sensor::configureOptions () {
	if (this-> _logPathOpt-> count () > 0) {
	    auto oldLogFile = this-> _logger.getLogFilePath ();
	    auto newLogFile = this-> _logPathOpt-> as<std::string> ();
	    if (oldLogFile != newLogFile) {
		this-> _logger.redirect (newLogFile, false);
	    }
	}

	if (this-> _logLvlOpt-> count () > 0) {
	    this-> _logger.changeLevel (this-> _logLvlOpt-> as<std::string> ());
	}

	if (this-> _freqOpt-> count () > 0) {
	    this-> _freq = 1.0 / this-> _freqOpt-> as<float> ();
	}
    }   

    void Sensor::configureFormula (const common::utils::config::dict & config) {
	auto success = this-> _factory.configurePlugin ("formula", config);

	if (success) {
	    auto formula = this-> _factory.getPlugins ("formula");
	    if (formula.size () == 1) {
		this-> _formula = formula[0];
		this-> _factory.forget ("formula");

		auto eventCall = this-> _formula-> getFunction<std::vector <std::string> (*)()> ("get_perf_events");
		if (eventCall == nullptr) {
		    this-> _logger.error ("Invalid formula plugin : ", this-> _formula-> getPath (), ", error : no function 'std::vector <std::string> get_perf_events ()'");
		    exit (-1);
		}

		this-> _events = eventCall ();
		this-> _formulaCompute = this-> _formula-> getFunction <FormulaComputeFunc_t> ("compute");
		if (this-> _formulaCompute == nullptr) {
		    this-> _logger.error ("Invalid formula plugin : ", this-> _formula-> getPath (), ", error : no function \n\t'void compute (const std::vector <common::cgroup::Cgroup> & cgroups, \n\t\t\tconst std::vector <std::vector<uint64_t> > & perfCounters, \n\t\t\tcommon::plugin::Factory & factory))'");
		    exit (-1);
		}

		this-> _formulaDump = this-> _formula-> getFunction <FormulaComputeFunc_t> ("dump");
		return;
	    }
	}
	
	this-> _logger.error ("Configuration of formula failed.");
	exit (-1);	
    }

    void Sensor::configurePerfEventWatchers () {
	this-> _cgroupWatchers.clear ();
	
	this-> _cgroupList = this-> readCgroupList ();
	this-> _cgroupList.insert (this-> _cgroupList.begin (), common::cgroup::Cgroup (""));
	for (auto & it : this-> _cgroupList) {
	    perf::PerfEventWatcher pw (it.getName ());
	    pw.configure (this-> _events);
	    this-> _cgroupWatchers.push_back (std::move (pw));
	    this-> _perfEventValues.push_back({});
	    this-> _perfEventValues.back ().resize (this-> _events.size ());
	}	
    }

    std::vector <cgroup::Cgroup> Sensor::readCgroupList () {       
	std::ifstream infile(this-> _cgroupFile);
	std::string line;
	std::vector <std::string> rules;
	while (std::getline(infile, line)) {
	    rules.push_back (line);	    
	}

	cgroup::CgroupLister lister (rules);
	return lister.run ();
    }

    void Sensor::configurePlugin (const std::string & kind, const common::utils::config::dict & config) {
	if (!this-> _factory.configurePlugin (kind, config)) {
	    this-> _logger.error ("Failed to configure plugin of kind ", kind, " : ", config);
	    exit (-1);
	}
	
	if (config.has <std::string> ("name")) {
	    auto name = config.get<std::string> ("name");
	    auto it = this-> _dumpingFiles.find (name);
	    if (it == this-> _dumpingFiles.end ()) {		
		this-> _dumpingFiles.emplace (name, std::ofstream (utils::join_path (VJOULE_DIR, "results/" + name + ".csv"), std::ios::app));
	    }
	}
    }
    
    

}
