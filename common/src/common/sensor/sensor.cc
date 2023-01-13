#include <common/sensor/sensor.hh>
#include <sys/sysinfo.h>

namespace common::sensor {

    Sensor::Sensor (const std::string & name, int argc, char ** argv) :
	_argc (argc),
	_argv (argv),
	_listeningAddr (std::string ("0.0.0.0:0")),
	_listener (net::SockAddrV4 {std::string ("0.0.0.0:0")}),
	_app (name)
    {}

    void Sensor::run () {
	this-> preRun ();
	
	this-> initAppOptions ();
	if (this-> _cfgPathOpt-> count () > 0) {
	    this-> _cfgPath = this-> _cfgPathOpt-> as<std::string> ();
	} else this-> _cfgPath = utils::join_path (VJOULE_DIR, "sensor/config.toml");

	this-> _acceptClients = false;
	this-> reconfigure ();
	
	this-> startListener ();
	this-> startMainLoop ();

	this-> join ();
	this-> dispose ();
    }


    void Sensor::preRun () {}
        
    /**
     * ===================================================================================
     * ===================================================================================
     * ================================   CONFIGURATION     ==============================
     * ===================================================================================
     * ===================================================================================
     */

    void Sensor::reconfigure () {
	auto oldAddr = this-> _listeningAddr;
	auto oldHead = this-> _head;
	this-> configure ();

	if (this-> _acceptClients && 
	    (oldAddr != this-> _listeningAddr ||
	     this-> _head != oldHead) 
	) {
	    utils::Logger::globalInstance ().debug ("Restart accept loop");
	    this-> closeListener ();
	    this-> startListener ();
	}
    }
    
    void Sensor::initAppOptions () {
	std::string configPath, logPath, logLevel;
	int port;
	float freq;
	
	this-> _cfgPathOpt = this-> _app.add_option ("-c,--config-path", configPath, "the path of the configuration file");
	this-> _logPathOpt = this-> _app.add_option ("-l,--log-path", logPath, "the path of the log file");
	this-> _logLvlOpt = this-> _app.add_option ("-v,--log-lvl", logLevel, "the lvl of the log");
	this-> _portOpt = this-> _app.add_option ("-p,--port", port, "the port of the sensor");
	this-> _freqOpt = this-> _app.add_option ("-f,--freq", freq, "the frequency of the sensor");
	this-> addCustomOptions (this-> _app);
	
	try {
	    this-> _app.parse (this-> _argc, this-> _argv);
	} catch (const CLI::ParseError &e) {
	    exit (this-> _app.exit (e));
	}
    }

    void Sensor::addCustomOptions (CLI::App&) {}
    
    void Sensor::configure () {
	this-> dispose ();

	utils::Logger::globalInstance ().info ("Search configuration file");
	auto configPath = utils::get_absolute_path_if_exists (this-> _cfgPath);
	if (configPath == "") {
	    utils::Logger::globalInstance ().debug ("No config file found at : ", this-> _cfgPath);
	    return;
	}
	
	auto config = utils::parse_file (configPath);
	auto cgroupConfig = config.getOr<utils::config::dict> ("cgroups", {});
	auto sensorConfig = config.getOr<utils::config::dict> ("sensor", {});
	auto eventConfig = config.getOr<utils::config::dict> ("events", {});

	this-> _freq = 1.0f / sensorConfig.getOr <float> ("freq", 1.0f);
	this-> _listeningAddr = sensorConfig.getOr<std::string> ("addr", std::string("0.0.0.0:0"));

	auto oldLogFile = utils::Logger::globalInstance ().getLogFilePath ();
	auto newLogFile = sensorConfig.getOr <std::string> ("log-path", "");
	if (oldLogFile != newLogFile) {
	    utils::Logger::globalInstance ().redirect (newLogFile);
	}
	utils::Logger::globalInstance ().changeLevel (sensorConfig.getOr<std::string> ("log-lvl", "SUCCESS"));
	
	this-> _systemEvents = this-> getEvents (eventConfig.getOr<utils::config::array> ("system", {}), {});//, {"PERF_COUNT_SW_CPU_CLOCK"});
	
	//this-> _systemEvents.push_back ("PERF_COUNT_SW_CPU_CLOCK"); // system event always have PERF_COUNT_SW_CPU_CLOCK and it is always the last one	
	this-> _cgroupEvents = this-> getEvents (eventConfig.getOr<utils::config::array> ("cgroups", {}), {}); //, {"PERF_COUNT_SW_CPU_CLOCK"});
	//this-> _cgroupEvents.push_back ("PERF_COUNT_SW_CPU_CLOCK"); // cgroup events always have PERF_COUNT_SW_CPU_CLOCK and it is always the last one

	this-> _head = this-> createHeader (this-> _systemEvents, this-> _cgroupEvents);
	this-> _head.nbCores = get_nprocs ();
	
	auto power = sensorConfig.getOr<utils::config::array> ("power-distr", {});
	this-> _head.powerByCpus.resize (power.size ());
	for (int i = 0 ; i < power.size () ; i++) {
	    this-> _head.powerByCpus [i] = power.get<float> (i);
	}
	
	this-> configureOptions ();
	this-> postConfigure (config);	
    }

    void Sensor::configureOptions () {
	if (this-> _logPathOpt-> count () > 0) {
	    auto oldLogFile = utils::Logger::globalInstance ().getLogFilePath ();
	    auto newLogFile = this-> _logPathOpt-> as<std::string> ();
	    if (oldLogFile != newLogFile) {
		utils::Logger::globalInstance ().redirect (newLogFile);
	    }
	}

	if (this-> _logLvlOpt-> count () > 0) {
	    utils::Logger::globalInstance ().changeLevel (this-> _logLvlOpt-> as<std::string> ());
	}

	if (this-> _portOpt-> count () > 0) {
	    auto port = this-> _portOpt-> as<int> ();
	    this-> _listeningAddr = std::string ("0.0.0.0:") + std::to_string (port);
	}

	if (this-> _freqOpt-> count () > 0) {
	    this-> _freq = 1.0 / this-> _freqOpt-> as<float> ();
	}
    }

    void Sensor::postConfigure (const common::utils::config::dict&) {}
    
    net::Header Sensor::createHeader (const std::vector <std::string> & systemEvents, const std::vector <std::string> & cgroupEvents) const {
	short id = 0;
	net::InversedHeader inv;
	for (auto & event : systemEvents) {
	    auto fnd = inv.metricIds.find (event);
	    if (fnd == inv.metricIds.end ()) {
		inv.metricIds.emplace (event, id);
		id += 1;
	    }
	}

	for (auto & event : cgroupEvents) {
	    auto fnd = inv.metricIds.find (event);
	    if (fnd == inv.metricIds.end ()) {
		inv.metricIds.emplace (event, id);
		id += 1;
	    }
	}

	auto ret = net::reverse (inv);
	ret.nbCores = get_nprocs ();

	return ret;
    }

    std::vector <std::string> Sensor::getEvents (const utils::config::array& array, const std::set <std::string> & ignore) const {
	std::vector <std::string> result;
	for (int i = 0 ; i < array.size () ; i++) {
	    try {
		std::string event = array.get<std::string> (i);
		if (ignore.find (event) == ignore.end ()) {
		    result.push_back (event);
		}
	    } catch (utils::config::config_error& err) {
		utils::Logger::globalInstance ().error ("While reading config file : ", array, " => ", err.getMessage ());
	    }
	}

	return result;
    }
    
    void Sensor::writePortFile (unsigned short port) {
	auto path = utils::join_path (VJOULE_DIR, "sensor/port");
	std::ofstream file (path, std::ios::out);
	file << port;

	file.close ();
    }
    
    /**
     * ===================================================================================
     * ===================================================================================
     * ===================================   CLIENTS      ================================
     * ===================================================================================
     * ===================================================================================
     */

    void Sensor::sendPacketToClients (const common::net::Packet & pack) {
	if (this-> _clients.size () > 0) {
	    net::writePacketToBytes (pack, this-> _cache); // we put that in cache so we don't reallocate each time    	    
	    this-> _clientMutex.lock ();
	    // Send the result to the connected formulas
	    std::vector <net::TcpStream> connected;
	    for (auto & client : this-> _clients) {
		if (client.sendPacket (this-> _cache)) {
		    connected.push_back (client);
		} else utils::Logger::globalInstance ().info ("Client disconnected : ", client.addr ());
	    }

	    // maybe some client were disconnected, no need to keep them indefinitely
	    this-> _clients = std::move (connected);	    
	    this-> _clientMutex.unlock ();
	    
	}
    }

    void Sensor::startListener () {
	this-> _acceptClients = true;
	this-> _acceptLoop = concurrency::spawn (this, &Sensor::acceptLoop);
    }


    void Sensor::closeListener () {
	this-> _acceptClients = false;
	this-> _clientMutex.lock (); // we lock the mutex to make sure the loop is not creating the listener
	net::TcpStream poisonPill (net::SockAddrV4 {net::Ipv4Address (127, 0, 0, 1), this-> _listener.port ()});
	this-> _clientMutex.unlock (); // we unlock it otherwise it will never accept the poison pill

	poisonPill.connect (); // send the poison pill 	    	    	    
	concurrency::join (this-> _acceptLoop); // wait for the accept loop to receive the pill
	
	for (auto & client : this-> _clients) {
	    client.close (); // close old connection
	}
	
	this-> _listener.close (); // close the tcp server
    }

    void Sensor::startMainLoop () {
	this-> _mainLoopRunning = true;
	this-> _mainLoop = concurrency::spawn (this, &Sensor::mainLoop);
    }

    void Sensor::closeMainLoop () {
	this-> _mainLoopRunning = false;
    }

    void Sensor::joinMainLoop () {
	concurrency::join (this-> _mainLoop);
    }
    
    void Sensor::mainLoop (common::concurrency::thread) {
	utils::Logger::globalInstance ().debug ("Starting main loop");
	concurrency::timer timer;
	while (this-> _mainLoopRunning) {
	    timer.reset ();
	    // this-> _mainLoopMutex.lock ();

	    this-> onUpdate ();

	    // this-> _mainLoopMutex.unlock ();
	    
	    auto took = timer.time_since_start ();
	    if (this-> _sleepActive) {
		float toSleep = this-> _freq - took - 0.001;
		if (toSleep > 0.0f) {
		    timer.sleep (toSleep);
		}
	    }
	    
	    utils::Logger::globalInstance ().debug ("Main loop iteration : ", took, "s");
	}
	utils::Logger::globalInstance ().debug ("Closing main loop");
    }


    void Sensor::acceptLoop (common::concurrency::thread) {
	this-> _clientMutex.lock ();
	
	this-> _listener.close ();
	this-> _listener = net::TcpListener (net::SockAddrV4 {this-> _listeningAddr});
	this-> _listener.start ();
	this-> _clientMutex.unlock ();
	
	utils::Logger::globalInstance ().info ("Waiting connection : ", this-> _listeningAddr.ip (), ":", this-> _listener.port ());

	// write the port file, so formulas can connect to it
	this-> writePortFile (this-> _listener.port ());
	
	while (true) { 
	    auto stream = this-> _listener.accept ();
	    if (this-> _acceptClients) { // false if the connection is a poison pill
		stream.send (this-> _head); 
		this-> _clientMutex.lock ();
		utils::Logger::globalInstance ().info ("Client connected : ", stream.addr ());	    
		this-> _clients.push_back (stream);
		this-> _clientMutex.unlock ();
	    } else {
		utils::Logger::globalInstance ().info ("Accept loop received poison pill !");
		break;
	    }
	}
    }

}
