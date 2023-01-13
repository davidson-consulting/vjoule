#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <set>

#include <common/utils/_.hh>
#include <common/net/_.hh>
#include <common/concurrency/_.hh>
#include <common/formula/divider.hh>
#include <common/formula/value.hh>
#include <common/foreign/CL11.hpp>
#include <sys/resource.h>

namespace common::formula {

    struct CgroupHandler {
	std::string name;
	PackageValue value;
	FILE* package;
	FILE* dram;
	FILE* pp0;
	FILE* pp1;
	FILE* psys;
    };
    
    /**
     * Main class of the simple formula
     */
    template <class DividerT>
    class Formula {
	static_assert (std::is_base_of<common::formula::Divider, DividerT>::value);
    private :

	// The connection socket to the sensor
	common::net::TcpStream _sensor;

	// The current header of the formula
	common::net::Header _header;

	// The consumption of the running cgroups
	std::vector <std::map <std::string, CgroupHandler> > _cgroupConso;

	// The list of cgroup that have to be removed 
	std::map <std::string, unsigned int> _toRemove;

	// The list of metrics to watch for cgroups
	std::vector <std::string> _cgroupMetrics;

	// The list of metrics to watch for system
	std::vector <std::string> _systemMetrics;

	// The name of the formula
	std::string _formulaName;
	
	// The divider of energy consumption
	std::map<RaplType, DividerT> _divider;

	/**
	 * ===================================================================================
	 * ===================================================================================
	 * ================================   POWER MODEL    =================================
	 * ===================================================================================
	 * ===================================================================================
	 */    
	
	// The power model used for simulation
	std::map <RaplType, std::vector <float> > _powerModels;

	// The meta model used to create power models
	std::vector <float> _metaPowerModel;
	
	// the id of the cpu clock metric
	short _cpuClockId;

	/**
	 * ===================================================================================
	 * ===================================================================================
	 * ================================   CONFIGURATION    ===============================
	 * ===================================================================================
	 * ===================================================================================
	 */    
	
	// The path of the result files
	std::string _mntPath;

	// true if the formula should reconfigure the sensor config file
	bool _reconfigureSensor;

	// Swipe cgroup memory if older than x seconds
	unsigned long _swipeAfter = 5;

	// True if port is passed by configuration
	bool _portIsForced = false;
	
	// The port of the sensor (if forced by config file)
	unsigned short _port;

	// The path of the config file
	std::string _configFilePath;

	// Quit when disconnected from the sensor
	bool _quitDisconnect = false;

	// The handle of the signal
	FILE* _signalFD = nullptr;
	
	// The argc of the program
	int _argc;
	
	// The argv of the program
	char ** _argv;

	// The option parser
	CLI::App _app;

	CLI::Option* _cfgPathOpt;
	CLI::Option* _mntOpt;
	CLI::Option* _logOpt;
	CLI::Option* _logLvlOpt;
	CLI::Option* _portOpt;
	CLI::Option* _swipOpt;
	CLI::Option* _reconfOpt;
	CLI::Option* _quitOpt;

	
    public :

	/**
	 * Create an empty formula
	 */
	Formula (const std::string & formulaName, int argc, char ** argv) :
	    _sensor (net::SockAddrV4 (std::string ("0.0.0.0:0"))),
	    _formulaName (formulaName),
	    _argc (argc),
	    _argv (argv)
	    {
		DividerT div (RaplType::DYN);
		
		this-> _systemMetrics = div.getSystemMetrics ();
		this-> _cgroupMetrics = div.getCgroupMetrics ();

		this-> _port = 0;
		this-> _portIsForced = false;
		this-> _reconfigureSensor = true;
		this-> _mntPath = utils::join_path (VJOULE_DIR, this-> _formulaName);
		this-> _configFilePath = utils::join_path (utils::join_path (VJOULE_DIR, this-> _formulaName), "config.toml");
		this-> _swipeAfter = 5;
		this-> _cgroupConso.resize (RaplType::END);
	    }

	/**
	 * Main loop of the formula
	 * @warning: infinite loop
	 */
	void run () {
	    this-> initAppOptions ();
	    if (this-> _cfgPathOpt-> count () > 0) {
		this-> _configFilePath = this-> _cfgPathOpt-> as<std::string> ();
	    }
	    
	    this-> configure ();
	    this-> configureArgs ();	    
	    
	    this-> configureSensor ();
	    
	    while (true) {
		this-> _sensor = this-> connectToSensor ();
		utils::Logger::globalInstance ().info ("Connected to sensor");
	    
		this-> _header = this-> _sensor.receiveHeader ();		
		this-> configureDividers ();
		if (this-> _header.simulation) {
		    this-> constructPowerModels ();
		}

		concurrency::timer t, formTimer;
		while (this-> _sensor.isOpen ()) {
		    t.reset ();		    
		    auto packet = this-> _sensor.receivePacket ();
		    
		    formTimer.reset ();
		    this-> treatPacket (t.time_since_start (), packet);
		    this-> memorySwipe ();
		    auto took = formTimer.time_since_start ();

		    utils::Logger::globalInstance ().debug ("Packet treated : ", t.time_since_start (), " took ", took, "ms for the formula");
		}
		
		if (this-> _quitDisconnect) break;
	    }

	    this-> dispose ();
	}
	
    private :
	
	/**
	 * ===================================================================================
	 * ===================================================================================
	 * ================================    POWER MODEL    ================================
	 * ===================================================================================
	 * ===================================================================================
	 */    

	/**
	 * Construct a power model from the power model of the formula
	 */
	std::vector <float> constructPowerModel (float pmax) {
	    if (this-> _metaPowerModel.size () == 0) return {};
	    std::vector <float> result (this-> _metaPowerModel.size ());
	    float percent = pmax / this-> _metaPowerModel.back ();
	    
	    for (int i = 0 ; i < this-> _metaPowerModel.size () ; i++) {
		result[i]  = this-> _metaPowerModel [i] * percent;
	    }

	    return result;
	}

	/**
	 * Construct the power models of the formula for simulations
	 */
	void constructPowerModels () {
	    this-> _powerModels.emplace (RaplType::MAX, this-> constructPowerModel (this-> _header.limits.PMAX));
	    this-> _powerModels.emplace (RaplType::MIN, this-> constructPowerModel (this-> _header.limits.PMIN));
	    this-> _powerModels.emplace (RaplType::MOY, this-> constructPowerModel (this-> _header.limits.PMOY));
	    this-> _cpuClockId = 0;
	    for (auto & it : this-> _header.metricNames) {
		if (it.second == "PERF_COUNT_SW_CPU_CLOCK") {
		    this-> _cpuClockId = it.first;
		}
	    }
	}

	/**
	 * Get the energy consumption for a given load in the powerModel
	 * @params: 
	 *    - load: the load between 0 and 1
	 *    - powerModel: the powermodel to use
	 */
	float getEnergy (float load, const std::vector <float> & powerModel) const {
	    if (powerModel.size () == 0) return 0;
	    
	    float floatIndex = load * (float) (powerModel.size () - 1);
	    int lowIndex = floor (floatIndex);
	    int highIndex = ceil (floatIndex);

	    float highValue = powerModel [highIndex];
	    float lowValue = powerModel [lowIndex];


	    float resultValue = (highValue - lowValue) * ((load * floatIndex) - lowIndex);

	    return resultValue + lowValue;	
	}
	
	/**
	 * Create package rapl from simulation
	 */
	void performRaplSimulation (RaplType type, common::net::Packet & packet, unsigned long currentCPUClock, float elapsed)  {
	    unsigned long nbMicros = 1000000; // number of micro second in a second
	    unsigned long maxLoad = this-> _header.nbCores * nbMicros * elapsed;

	    auto load = (float) currentCPUClock / (float) maxLoad;
	    packet.energy_pkg = this-> getEnergy (load, this-> _powerModels [type]);	    
	}

	/**
	 * Find the system cpu clock in the packet sent by the sensor
	 * @returns: the value, 0 if not found
	 */
	unsigned long findCPUClock (const common::net::Packet & packet) const {
	    for (auto & it : packet.globalMetrics) {
		if (it.id == this-> _cpuClockId) {
		    return it.value;
		}	    
	    }
	    
	    return 0;
	}
	

	/**
	 * ===================================================================================
	 * ===================================================================================
	 * ================================   DIVISION     ===================================
	 * ===================================================================================
	 * ===================================================================================
	 */    

	/**
	 * Treat a packet
	 * @params: 
	 *    - frameDur: the elapsed time since last tick
	 *    - packet: the packet to treat
	 */
	void treatPacket (float elapsed, common::net::Packet & packet) {
	    unsigned long cpu_clock;
	    if (this-> _header.simulation) {
		cpu_clock = this-> findCPUClock (packet);
	    }
	    
	    for (auto & it : this-> _divider) {
		if (it.first != RaplType::DYN) {
		    this-> performRaplSimulation (it.first, packet, cpu_clock, elapsed);
		}
		
		auto res = it.second.divideConsumption (packet); // divide the consumption of the packet	
		this-> exportResults (it.first, res);		
	    }
	}
	
	/**
	 * ===================================================================================
	 * ===================================================================================
	 * ================================   CONFIGURATION    ===============================
	 * ===================================================================================
	 * ===================================================================================
	 */    
		
	/**
	 * Configure the formula from the configuration file
	 */
	void configure () {
	    if (this-> _configFilePath == "") return;
	    
	    utils::Logger::globalInstance ().info ("Search configuration file");	
	    auto configPath = utils::get_absolute_path_if_exists (this-> _configFilePath);
	    if (configPath == "") {
		utils::Logger::globalInstance ().warn ("Configuration file not found at : ", this-> _configFilePath);
		return;
	    }

	    auto config = utils::parse_file (configPath);
	    auto formulaConfig = config.getOr<utils::config::dict> ("formula", {});

	    auto nbMaxOpen = formulaConfig.getOr <int> ("max-open-files", 65535);
	    struct rlimit r;
	    r.rlim_cur = nbMaxOpen;
	    r.rlim_max = nbMaxOpen;

	    utils::Logger::globalInstance ().info ("Setting max open files : ", nbMaxOpen);
	    setrlimit (RLIMIT_NOFILE, &r);
	    getrlimit (RLIMIT_NOFILE, &r);

	    if (r.rlim_cur != nbMaxOpen) {
		utils::Logger::globalInstance ().error ("Failed setting max open files, current max is : ", r.rlim_cur);
	    }
	    
	    if (this-> _logOpt-> count () > 0) {
		utils::Logger::globalInstance ().redirect (this-> _logOpt-> as<std::string> ());
	    } else {
		// log configuration
		auto oldLogFile = utils::Logger::globalInstance ().getLogFilePath ();
		auto newLogFile = formulaConfig.getOr <std::string> ("log-path", "");
		if (oldLogFile != newLogFile) {
		    utils::Logger::globalInstance ().redirect (newLogFile);
		}
	    }
	    
	    if (this-> _logLvlOpt-> count () > 0) {
		utils::Logger::globalInstance ().changeLevel (this-> _logLvlOpt-> as<std::string> ());
	    } else {
		utils::Logger::globalInstance ().changeLevel (formulaConfig.getOr<std::string> ("log-lvl", "SUCCESS"));
	    }
	    

	    // path where the result are written
	    this-> _mntPath = formulaConfig.getOr<std::string> ("mnt-path", utils::join_path (VJOULE_DIR, this-> _formulaName));

	    if (formulaConfig.has<int> ("port")) {
		this-> _port = formulaConfig.get<int> ("port");
		this-> _portIsForced = true;
	    }
	    
	    // should we reconfigure the sensor metrics ?
	    this-> _reconfigureSensor = formulaConfig.getOr<bool> ("reconf-sensor", true);
	    this-> _swipeAfter = formulaConfig.getOr<int> ("swipe-after", this-> _swipeAfter);

	    this-> _signalFD = fopen (utils::join_path (this-> _mntPath, "formula.signal").c_str (), "w");
	    if (this-> _signalFD == nullptr) {
		utils::Logger::globalInstance ().error ("Failed to open ", utils::join_path (this-> _mntPath, "formula.signal"), " permission denied");
		exit (-1);
	    }
	}

	/**
	 * Initialize the options of the application (command line options)
	 */
	void initAppOptions () {
	    this-> _cfgPathOpt = this-> _app.add_option ("-c,--config-path", "the path of the configuration file");
	    this-> _mntOpt = this-> _app.add_option ("-m,--mnt-path", "the mount directory of the formula");
	    this-> _logOpt = this-> _app.add_option ("-l,--log-path", "the path of the log file");
	    this-> _logLvlOpt = this-> _app.add_option ("-v, --log-lvl", "the lvl of the log");
	    this-> _portOpt = this-> _app.add_option ("-p,--port", "the port of the sensor");
	    this-> _swipOpt = this-> _app.add_option ("-s,--swipe-after", "number of iteration to wait before cgroup result deletion");

	    bool dummy;
	    this-> _reconfOpt = this-> _app.add_flag ("-n,--no-sensor-reconf", dummy, "don't reconfigure the sensor");
	    this-> _quitOpt = this-> _app.add_flag ("-q,--quit-disconnect", dummy, "quit on sensor disconnection");

	    try {
		this-> _app.parse (this-> _argc, this-> _argv);
	    } catch (const CLI::ParseError &e) {
		exit (this-> _app.exit (e));
	    }
	}
	
	/**
	 * Configure the formula form the program arguments
	 */
	void configureArgs () {
	    if (this-> _mntOpt-> count () > 0) {
		this-> _mntPath = this-> _mntOpt-> as<std::string> ();
	    }

	    if (this-> _swipOpt-> count () > 0) {
		this-> _swipeAfter = this-> _swipOpt-> as<int> ();
	    }

	    if (this-> _portOpt-> count () > 0) {
		this-> _port = this-> _portOpt-> as<int> ();
		this-> _portIsForced = true;
	    }

	    if (this-> _reconfOpt-> count () > 0) {
		this-> _reconfigureSensor = false;
	    }

	    if (this-> _quitOpt-> count () > 0) {
		this-> _quitDisconnect = true;
	    }
	    
	    if (this-> _logOpt-> count () > 0) {
		utils::Logger::globalInstance ().redirect (this-> _logOpt-> as<std::string> ());
	    }
	    
	    if (this-> _logLvlOpt-> count () > 0) {
		utils::Logger::globalInstance ().changeLevel (this-> _logLvlOpt-> as<std::string> ());
	    }
	}
	
	
	/**
	 * Modify the configuration of the sensor so that it sends the correct metrics
	 */
	void configureSensor () const {
	    if (this-> _reconfigureSensor) {
		auto configPath = utils::get_absolute_path("sensor/config.toml"); // we modify the sensor config file, so it has the correct metrics for the formula

		utils::config::dict config = utils::parse_file (configPath);	    
		utils::config::array cgroupMetrics (this-> _cgroupMetrics);
		utils::config::array globalMetrics (this-> _systemMetrics); 
	    	    
		utils::config::dict system;
		system.insert ("system", new utils::config::array (globalMetrics));
		system.insert ("cgroups", new utils::config::array (cgroupMetrics));

		config.insert ("events", new utils::config::dict (system));

		auto formatted = utils::format (config);

		std::ofstream file (configPath, std::ios::out); // write the configuration file where we read it
		file << formatted;

		file.close ();
	    }
	}


	void configureDividers () {
	    if (this-> _header.simulation) {
		if (this-> _divider.size () != RaplType::END - 1) {
		    this-> _divider.clear ();
		    for (int i = RaplType::DYN + 1; i < RaplType::END ; i++) {
			this-> _divider.emplace ((RaplType) (RaplType::DYN + i), DividerT {((RaplType) (RaplType::DYN + i))});
		    }
		}
	    } else {
		if (this-> _divider.size () != 1) {
		    this-> _divider.clear ();
		    auto div = DividerT (RaplType::DYN);
		    this-> _divider.emplace (RaplType::DYN, DividerT {RaplType::DYN});
		}
	    }

	    for (auto & it : this-> _divider) {
		it.second.configure (this-> _header);
	    }
	}

	
	void dispose () {
	    if (this-> _signalFD != nullptr) {
		fclose (this-> _signalFD);
		this-> _signalFD = nullptr;
	    }

	    for (auto & vt : this-> _cgroupConso) {
		for (auto & it : vt) { 
		    this-> closeCgroup (it.second);
		}
	    }
	    
	    this-> _cgroupConso.clear ();
	}
	
	/**
	 * ===================================================================================
	 * ===================================================================================
	 * ================================   EXPORTING     ==================================
	 * ===================================================================================
	 * ===================================================================================
	 */    
	
	/**
	 * Export the results of a division
	 */
	void exportResults (RaplType type, const std::map <std::string, PackageValue> & res) {
	    for (auto & it : res) {
		auto fnd = this-> _cgroupConso[type].find (it.first);
		if (fnd != this-> _cgroupConso[type].end ()) { // this is an old cgroup, we cumulate the consumption    
		    this-> writeCgroup (fnd-> second, it.second);
		} else { // this is a new cgroup
		    auto cgroupName = simplifyName (it.first);
		    auto path = utils::join_path (this-> _mntPath, cgroupName);
		    this-> _cgroupConso[type].emplace (it.first, this-> openCgroup (type, cgroupName, path, it.second));

		    utils::Logger::globalInstance ().info ("Create result directory for cgroup : ", cgroupName);
		}
	    }

	    for (auto & it : this-> _cgroupConso[type]) { // we traverse all managed cgroups by the formula  	    
		auto still = res.find (it.first);
		if (still == res.end ()) { // last packet did not contain this cgroup
		    auto cgroupName = simplifyName (it.first);
		    auto toRM = this-> _toRemove.find (cgroupName);
		    if (toRM == this-> _toRemove.end ()) {
			this-> _toRemove.emplace (cgroupName, 0); // the cgroup is inactive, maybe it doesn't exist anymore
		    }
		} else {
		    // the cgroup is still running
		    auto cgroupName = simplifyName (it.first);
		    this-> _toRemove.erase (cgroupName); // the cgroup is active, we don't want to remove it
		}
	    }

	    fseek (this-> _signalFD, 0, SEEK_SET);
	    int i = 1;
	    fwrite (&i, sizeof (int), 1, this-> _signalFD);
	    fflush (this-> _signalFD);
	}

	/**
	 * Open cgroup files to manage it in the future
	 */
	CgroupHandler openCgroup (RaplType type, const std::string & name, const std::string & path, PackageValue val) {
	    std::filesystem::create_directories (path);
	    std::string endName = "";
	    switch (type) {
	    case RaplType::MAX : endName = "_max"; break;
	    case RaplType::MIN : endName = "_min"; break;
	    case RaplType::MOY : endName = "_moy"; break;
	    }
	    
	    auto package = fopen (utils::join_path (path, "package" + endName).c_str (), "w");
	    auto memory = fopen (utils::join_path (path, "memory" + endName).c_str (), "w");
	    auto pp0 = fopen (utils::join_path (path, "pp0" + endName).c_str (), "w");
	    auto pp1 = fopen (utils::join_path (path, "pp1" + endName).c_str (), "w");
	    auto psys = fopen (utils::join_path (path, "psys" + endName).c_str (), "w");

	    // we write into package to make sure it exists
	    fprintf (package, "%lf", 0.0);
	    fflush (package);
	    
	    return {name, val, package, memory, pp0, pp1, psys};
	}
	
	/**
	 * Update the files of a cgroup
	 * @params: 
	 *    - c: the cgroup handler to update
	 */
	void writeCgroup (CgroupHandler & c, PackageValue v) {
	    if (v.package != 0) {
		c.value.package += v.package;
		fseek (c.package, 0, SEEK_SET);
		fprintf (c.package, "%lf", c.value.package);
		fflush (c.package);
		utils::Logger::globalInstance ().debug ("Write conso : ", c.name, " package : ", c.value.package);
	    }

	    if (v.dram != 0) {
		c.value.dram += v.dram;
		fseek (c.dram, 0, SEEK_SET);
		fprintf (c.dram, "%lf", c.value.dram);
		fflush (c.dram);
	    }

	    if (v.pp0 != 0) {
		c.value.pp0 += v.pp0;
		fseek (c.pp0, 0, SEEK_SET);
		fprintf (c.pp0, "%lf", c.value.pp0);
		fflush (c.pp0);
	    }
	    
	    if (v.pp1 != 0) {
		c.value.pp1 += v.pp1;
		fseek (c.pp1, 0, SEEK_SET);
		fprintf (c.pp1, "%lf", c.value.pp1);
		fflush (c.pp1);
	    }

	    if (v.psys != 0) {
		c.value.psys += v.psys;
		fseek (c.psys, 0, SEEK_SET);
		fprintf (c.psys, "%lf", c.value.psys);
		fflush (c.psys);
	    }	    
	}

	/**
	 * Close the files of a cgroup
	 */
	void closeCgroup (const CgroupHandler & c) {
	    fclose (c.package);
	    fclose (c.dram);
	    fclose (c.pp0);
	    fclose (c.pp1);
	    fclose (c.psys);
	}
	   
	/**
	 * Simplify a cgroup name for easier export reading
	 * This function can be used to make sure exported names have a readable name (e.g. vms with qemu have strange name, docker containers too, etc) 
	 * @returns: the simplified version of the name (should conserve the path though)
	 */
	std::string simplifyName (const std::string & name) const {
	    if (name.find ("machine-qemu") != std::string::npos) {
		auto pos = name.find ("machine-qemu");
		auto pos2 = name.rfind ("\\x2d") + 4;
		auto pos3 = name.rfind (".scope");

	    
		auto res = name.substr (0, pos) + name.substr (pos2, pos3 - pos2);
		return res;
	    }

	    return name;
	}

	/**
	 * ===================================================================================
	 * ===================================================================================
	 * ================================   MEMORY SWIPE     ===============================
	 * ===================================================================================
	 * ===================================================================================
	 */

	/**
	 * Remove all old cgroup results older than a certain amount of time
	 * The idea is that sensor may be reconfigured and some cgroup not read for a tick or two, but we don't wan't to remove their results in that case
	 */
	void memorySwipe () {
	    auto currentTime = (unsigned long)time (NULL);
	    std::map <std::string, unsigned int> res;
	    for (auto & itr : this-> _toRemove) {
		if (itr.second > this-> _swipeAfter) {
		    auto path = utils::join_path (this-> _mntPath, itr.first);
		    this-> removeCgroupDir (path);

		    common::utils::Logger::globalInstance ().info ("Remove cgroup path : ", itr.first, " age : ", itr.second, " > ", this-> _swipeAfter);

		    for (auto & type : this-> _cgroupConso) {
			auto it = type.find (itr.first);
			if (it != type.end ()) {
			    this-> closeCgroup (it-> second);
			}
			type.erase (itr.first);
		    }
		    
		} else {
		    common::utils::Logger::globalInstance ().info ("Not old enough : ", itr.first, " age : ", itr.second, " < ", this-> _swipeAfter);
		    res.emplace (itr.first, itr.second + 1);
		}	       
	    }

	    this-> _toRemove = res;
	}

	/**
	 * Remove a cgroup directory
	 * @params: 
	 *    - path: the path to the cgroup dir to remove
	 */
	void removeCgroupDir (const std::string & path) {
	    for (int i = RaplType::DYN ; i < RaplType::END ; i++) {
		std::string endName = "";
		switch (i) {
		case RaplType::MAX : endName = "_max"; break;
		case RaplType::MIN : endName = "_min"; break;
		case RaplType::MOY : endName = "_moy"; break;
		}
		
		auto ram_path = utils::join_path (path, "memory" + endName);
		auto package_path = utils::join_path (path, "package" + endName);
		auto pp0_path = utils::join_path (path, "pp0" + endName);
		auto pp1_path = utils::join_path (path, "pp1" + endName);
		auto psys_path = utils::join_path (path, "psys" + endName);
			    
		std::filesystem::remove (ram_path);
		std::filesystem::remove (package_path);
		std::filesystem::remove (pp0_path);
		std::filesystem::remove (pp1_path);
		std::filesystem::remove (psys_path);
	    }
	    
	    std::filesystem::remove (path);
	}
	
	
	/**
	 * ===================================================================================
	 * ===================================================================================
	 * ================================   CONNEXION     ==================================
	 * ===================================================================================
	 * ===================================================================================
	 */    
    	
	/**
	 * @returns: the port on which the sensor is running
	 * @warning: loop until there is sensor port available
	 */
	unsigned short getSensorPort () const {
	    if (!this-> _portIsForced) {
		auto file = utils::get_absolute_path_if_exists ("sensor/port");
		if (file != "") { // if the file was written by the sensor
		    std::ifstream t(file);
		    std::stringstream buffer;
		    buffer << t.rdbuf();
		    
		    unsigned short port;
		    buffer >> port; // read the port from it
		    
		    return port;
		} // no file found, sensor is not open 
	    }
	    
	    return this-> _port; 
	}

	/**
	 * Connect the formula to the running sensor
	 * @warning: loop until the connection succeeds
	 */
	common::net::TcpStream connectToSensor () {
	    concurrency::timer timer;
	    while (true) {
		// Retreive the port of the sensor, should be in /etc/vjoule/sensor/port
		auto port = this-> getSensorPort ();
		if (port != 0) {
		    utils::Logger::globalInstance ().debug ("Try connect to sensor at port : ", port);
		    net::TcpStream stream (net::SockAddrV4 {port}); 
		    if (stream.connect ()) { // try to connect to it
			return stream;
		    }
		}

		utils::Logger::globalInstance ().error ("Failed to connect to sensor, wait 1 second");
		// sleep 1 sec to wait for the sensor to be available
		timer.sleep (1);
	    }
	}
	    

	
    };
    

}
