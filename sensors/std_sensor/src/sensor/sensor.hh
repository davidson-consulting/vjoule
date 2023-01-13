#pragma once

#include <sensor/_.hh>
#include <common/_.hh>
#include <vector>
#include <string>

namespace sensor {

    /**
     * This is the main part of the sensor
     */
    class Sensor : public common::sensor::Sensor {
    private :
	
	// The report 
	common::net::Packet _report;

	// The notifier used to update the sensor when the system context has changed (cgroups, config)
	sensor::Notifier _notifier;

	// The watcher of the system metrics
	perf::PerfEventWatcher _systemWatcher;

	// The watchers of the cgroup metrics
	std::vector <perf::PerfEventWatcher> _cgroupWatchers;

	// The rapl value reader
	perf::RaplReader _raplReader;
	
    public :

	/**
	 * Create the sensor
	 * It will seek its configuration from the local directory or ${VJOULE_SENSOR_DIR}
	 */
	Sensor (int argc, char** argv);

	/**
	 * Function called at the start of the sensor
	 */
	void preRun () override;

	/**
	 * Configure some custom elements for this specific sensor
	 */
	void postConfigure (const common::utils::config::dict & cfg) override;

	/**
	 * Function called in the main loop
	 */
	void onUpdate () override;

	/**
	 * Join the loop
	 */
	void join () override;

	/**
	 * Dispose the sensor
	 */
	void dispose () override;
	
    private :

	/**
	 * Create the packet
	 */
	common::net::Packet createPacket (const common::net::Header& head, const std::string& cgroupMnt, const std::vector <cgroup::Cgroup>& cgroups, const std::vector<std::string> & systemEvents, const std::vector <std::string>& cgroupEvents);
	
	/**
	 * @slot
	 * This is the slot called by the notifier when there is a modification of the context (cgroup, config)
	 */
	void onContextChange ();
		
    };
    

}
