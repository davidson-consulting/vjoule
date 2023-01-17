#include <sensor/sensor.hh>
#include <common/net/ports.hh>
#include <fstream>
#include <sys/resource.h>
#include <sys/sysinfo.h>

using namespace common;

namespace sensor {

    Sensor::Sensor (int argc, char ** argv) :
	common::sensor::Sensor ("std_sensor", argc, argv),
	_systemWatcher ("/sys/fs/cgroup/")
    {}

    void Sensor::preRun () {
	pfm_initialize ();
    }

    void Sensor::join () {
	this-> _notifier.onUpdate ().connect (this, &Sensor::onContextChange);
	this-> _notifier.startSync (); // make sure the notifier is in the main thread

	pfm_terminate ();
    }

    /**
     * ===================================================================================
     * ===================================================================================
     * ================================   CONTEXT CHANGE     =============================
     * ===================================================================================
     * ===================================================================================
     */    
    
    void Sensor::onContextChange () { // this function is called by the notifier that is running in the main thread
	utils::Logger::globalInstance ().info ("Context changed");

	this-> closeMainLoop ();
	this-> joinMainLoop ();
	
	// We reconfigure the sensor, to update the perf watchers
	this-> reconfigure ();

	utils::Logger::globalInstance ().info ("Finished configure");

	// We restart the sensor main loop
	this-> startMainLoop ();
    }


    /**
     * ===================================================================================
     * ===================================================================================
     * ================================   CONFIGURATION     ==============================
     * ===================================================================================
     * ===================================================================================
     */    

    void Sensor::dispose () {
	this-> _raplReader.dispose ();
	this-> _systemWatcher.dispose ();
	this-> _cgroupWatchers.clear ();
    }
    
    void Sensor::postConfigure (const common::utils::config::dict & config) {
	auto cgroupConfig = config.getOr<utils::config::dict> ("cgroups", {});
	auto sensorConfig = config.getOr<utils::config::dict> ("sensor", {});
	auto eventConfig = config.getOr<utils::config::dict> ("events", {});
	auto raplConfig = config.getOr<utils::config::dict> ("rapl", {});

	auto nbMaxOpen = sensorConfig.getOr <int> ("max-open-files", 65535);
	struct rlimit r;
	r.rlim_cur = nbMaxOpen;
	r.rlim_max = nbMaxOpen;

	utils::Logger::globalInstance ().info ("Setting max open files : ", nbMaxOpen);
	setrlimit (RLIMIT_NOFILE, &r);
	getrlimit (RLIMIT_NOFILE, &r);

	if (r.rlim_cur != nbMaxOpen) {
	    utils::Logger::globalInstance ().error ("Failed setting max open files, current max is : ", r.rlim_cur);
	}

	this-> _raplReader.configure (raplConfig);
	if (this-> _raplReader.getMode () == perf::RaplReaderMode::NFS) {
	    this-> _systemEvents = this-> _raplReader.readSystemEvents ();
	    
	    this-> _head = this-> createHeader (this-> _systemEvents, this-> _cgroupEvents);
	    this-> _head.nbCores = get_nprocs ();
	    this-> _head.simulation = true;
	    
	    this-> _raplReader.readPowerLimits (this-> _head.limits.PMIN,
						this-> _head.limits.PMOY,
						this-> _head.limits.PMAX);
	
	    auto power = sensorConfig.getOr<utils::config::array> ("power-distr", {});
	    this-> _head.powerByCpus.resize (power.size ());
	    for (int i = 0 ; i < power.size () ; i++) {
		this-> _head.powerByCpus [i] = power.get<float> (i);
	    }	
	}
	
	this-> _sleepActive = (this-> _raplReader.getMode () == perf::RaplReaderMode::BARE_METAL);
	
	auto cgroupMnt = cgroupConfig.getOr<std::string> ("mnt", "/sys/fs/cgroup");
	this-> _notifier.configure (cgroupMnt, utils::parent_directory (this-> _cfgPath));
	this-> _systemWatcher.configure (this-> _systemEvents);

	::sensor::cgroup::CgroupLister lister (cgroupConfig);	
	auto listCgroups = lister.run ();
	this-> _report = this-> createPacket (this-> _head, cgroupMnt, listCgroups, this-> _systemEvents, this-> _cgroupEvents);
	
	for (auto & cgroup : listCgroups) {
	    perf::PerfEventWatcher watcher (cgroup.getName ());
	    watcher.configure (this-> _cgroupEvents);
	    this-> _cgroupWatchers.push_back (std::move (watcher));
	}
    }
    

    net::Packet Sensor::createPacket (const net::Header& head, const std::string& cgroupMnt, const std::vector <cgroup::Cgroup>& cgroups, const std::vector<std::string> & systemEvents, const std::vector <std::string>& cgroupEvents) {
	net::Packet pack;
	std::vector <net::Metric> cgroupMetrics;
	auto inverted = net::reverse (head);
	for (auto & event : systemEvents) {
	    auto id = inverted.metricIds.find (event);
	    pack.globalMetrics.push_back (net::Metric {id-> second, 0});
	}

	for (auto & event : cgroupEvents) {
	    auto id = inverted.metricIds.find (event);
	    cgroupMetrics.push_back (net::Metric {id-> second, 0});
	}
	
	for (auto & cgroup : cgroups) {
	    auto name = cgroup.getName ();
	    name = name.erase (0, cgroupMnt.length ());
	    pack.cgroupPackets.push_back (net::CgroupPacket {name, cgroupMetrics});
	}
	
	return pack;
    }

    /**
     * ===================================================================================
     * ===================================================================================
     * ================================   THREAD LOOPS     ===============================
     * ===================================================================================
     * ===================================================================================
     */    

    void Sensor::onUpdate () {
	// Poll the rapl values
	this-> _raplReader.poll (this-> _report);
	
	this-> _systemWatcher.poll (this-> _report.globalMetrics);

	for (int i = 0; i < this-> _report.cgroupPackets.size () ; i++) { 
	    auto & packet = this-> _report.cgroupPackets [i];
	    auto & watcher = this-> _cgroupWatchers[i];
	    watcher.poll (packet.metrics); // we poll directly in the preallocated packet for efficiency
	}

	this-> sendPacketToClients (this-> _report);
    }       
    
    
}
