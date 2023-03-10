#pragma once

#include <string>
#include <vector>
#include <linux/perf_event.h>
#include <perfmon/pfmlib_perf_event.h>
#include <perfmon/pfmlib.h>
#include <cstdint>

namespace common::perf {

    /**
     * Watch the performance event associated to a cgroup
     */
    class PerfEventWatcher {
    private :
	
	// the list of group head events
	std::vector <int> _fds;

	// The list of opened perf events
	std::vector<int> _toClose;
	
	// The list of events being watched
	std::vector <std::string> _eventList;

	// The cache to read the perf events
	std::vector <uint64_t> _cache;
	
	// The handle of the cgroup file descriptor
	int _cgroupFd;

	// The path to the cgroup to watch
	std::string _cgroupPath;

	// The date of the cgroup directory
	uint64_t _fileDate = 0;
	
    private :

	PerfEventWatcher (const PerfEventWatcher & other);

	void operator=(const PerfEventWatcher& other);
	
    public :

	/**
	 * Create an empty cgroup event watcher
	 * @warning: does not work until configure is not called
	 */
	PerfEventWatcher (const std::string & cgroupPath);

	/**
	 * Move ctor
	 */
	PerfEventWatcher (PerfEventWatcher&& other);

	/**
	 * Move affect
	 */
	void operator= (PerfEventWatcher&& other);

	/**
	 * ===================================================================================
	 * ===================================================================================
	 * ================================   CONFIGURE EVENTS     ===========================
	 * ===================================================================================
	 * ===================================================================================
	 */    
    
	
	/**
	 * Configure the perf event watcher to watch a list of events
	 * @params: 
	 *    - eventList: the list of event to watch
	 */
	void configure (const std::vector<std::string>& eventList);

	/**
	 * Reconfigure the watcher after a possible change of the cgroup directory
	 */
	void reconfigure (const std::vector<std::string>& eventList);
	
	/**
	 * ===================================================================================
	 * ===================================================================================
	 * ================================   POLLING EVENTS     =============================
	 * ===================================================================================
	 * ===================================================================================
	 */    
	
	/**
	 * Read the values inside the performance counters
	 * @returns: 
	 *   - returns: the value associated with the events
	 */
	void poll (std::vector <uint64_t> & values);
	
	/**
	 * ===================================================================================
	 * ===================================================================================
	 * ================================   GETTERS     ====================================
	 * ===================================================================================
	 * ===================================================================================
	 */    
    	
	/**
	 * @returns: the list of event being watched
	 */
	const std::vector<std::string>& getEventList () const;

	/**
	 * @returns: the name of the watched cgroup
	 */
	const std::string& getCgroupName () const;

	/**
	 * ===================================================================================
	 * ===================================================================================
	 * ================================   CLEARING EVENTS     ============================
	 * ===================================================================================
	 * ===================================================================================
	 */
	
	/**
	 * Dispose the event watcher
	 */
	void dispose ();

	/**
	 * Clear everything
	 */
	static void clear ();
	
	/**
	 * Call dispose
	 */
	~PerfEventWatcher ();

    private :

	/**
	 * Configure the watcher to watch the event from 'eventList' on nbCpus
	 * @params: 
	 *   - nbCpus: the number of cpus to watch
	 *   - eventList: the list of event to watch
	 */
	void configureCgroupWatch (const std::vector <std::string> & eventList, int nbCpus);

	/**
	 * Find the perf event associated to the names of eventList
	 * @params: 
	 *   - eventList: the names of the event to find
	 * @returns: 
	 *    - saved: the list of event that where found
	 *    - the perf event attributes that can be opened 
	 * @info: @return and saved are in the same order
	 */
	std::vector <perf_event_attr> findPerfEventAttrs (const std::vector <std::string> & eventList, std::vector<std::string> & saved);

	/**
	 * Find a perf event from its name
	 * @params:
	 *    - name: the name of the perf event
	 * @returns: a perf event that can be opened
	 */
	perf_event_attr findPerfEvent (const std::string & name) const;

	/**
	 * @returns: the date of the file
	 */
	uint64_t getCgroupDate () const;
	
    };
    

    
}

