#include <sys/sysinfo.h>
#include <fcntl.h>

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <asm/unistd.h>
#include <common/concurrency/timer.hh>
#include <common/utils/_.hh>
#include <watch.hh>

using namespace common::utils;

/**
 * Struct util to read the values from the event descriptors
 */
struct read_value {
    uint64_t value;
    uint64_t id;
};

/**
 * Struct util to read the values from the event descriptors
 */
struct read_format {
    uint64_t nr;
    read_value values[];
};


namespace tools::vjoule {

    bool __PERF_IS_INIT__ = false;
    
    PerfEventWatcher::PerfEventWatcher (const std::string & cgroupPath) :
	_cgroupPath (cgroupPath),
	_cgroupFd (0)
    {
    }
    
    PerfEventWatcher::PerfEventWatcher (PerfEventWatcher&& other) :
	_cgroupPath (other._cgroupPath),
	_cgroupFd (other._cgroupFd),
	_eventList (std::move (other._eventList)),
	_fds (std::move (other._fds)),
	_cache (std::move (other._cache)),
	_cacheSum (std::move (other._cacheSum)),
	_toClose (std::move (other._toClose))
    {
	other._cgroupFd = 0; // make sure that the move other does not close the cgroup file descriptor
    }

    void PerfEventWatcher::operator= (PerfEventWatcher&& other) {
	this-> dispose ();

	this-> _cgroupFd = other._cgroupFd;
	this-> _cgroupPath = other._cgroupPath;
	this-> _fds = std::move (other._fds);
	this-> _eventList = std::move (other._eventList);
	this-> _cacheSum = std::move (other._cacheSum);
	this-> _cache = std::move (other._cache);
	this-> _toClose = std::move (other._toClose);

	other._cgroupFd = 0; // make sure that the move other does not close the cgroup file descriptor
    }

    PerfEventWatcher::~PerfEventWatcher () {
	this-> dispose (); // only the real owner will close something
    }

    /**
     * ===================================================================================
     * ===================================================================================
     * ================================   CONFIGURE EVENTS     ===========================
     * ===================================================================================
     * ===================================================================================
     */    
    
    void PerfEventWatcher::configure (const std::vector <std::string> & eventList) {
	this-> dispose ();
	
	if (this-> _cgroupPath == "") this-> configureCgroupWatch (eventList, 1);
	else {
	    this-> configureCgroupWatch (eventList, get_nprocs ());
	}

	this-> _cache.resize (this-> _eventList.size () * sizeof (read_value) + sizeof (uint64_t));
	this-> _cacheSum.resize (this-> _eventList.size ());
    }
   
    void PerfEventWatcher::configureCgroupWatch (const std::vector <std::string> & eventList, int nbCpus) {
	int perfFlags = 0;
	if (this-> _cgroupPath != "") {
	    this-> _cgroupFd = open (this-> _cgroupPath.c_str (), O_RDONLY);
	    if (this-> _cgroupFd == -1) {
		common::utils::Logger::globalInstance  ().error ("Failed to open cgroup.");
		return;
	    }
	    perfFlags = PERF_FLAG_PID_CGROUP;
	} else {
	    this-> _cgroupFd = -1;	    
	}

	std::vector <perf_event_attr> attrs = this-> findPerfEventAttrs (eventList, this-> _eventList);
	
	for (int cpuId = 0 ; cpuId < nbCpus; cpuId++) {
	    int root = -1;
	    for (auto & pe : attrs) {
		auto fd = perf_event_open (&pe, this-> _cgroupFd, cpuId, root, perfFlags);		
		if (root == -1) {
		    root = fd;
		} else this-> _toClose.push_back (fd);
		
		if (fd == -1) {
		  common::utils::Logger::globalInstance ().error ("Failed to open perf event for cgroup : ", this-> _cgroupPath, " on cpu : ", cpuId, " ", strerror (errno));
		    continue;
		}
	    }

 	    if (root != -1) {
		ioctl(root, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
		ioctl(root, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);
		this-> _fds.push_back (root);
	    }
	}

    }

    /**
     * ===================================================================================
     * ===================================================================================
     * ================================   FIND EVENTS     ================================
     * ===================================================================================
     * ===================================================================================
     */    

    std::vector <perf_event_attr> PerfEventWatcher::findPerfEventAttrs (const std::vector<std::string> & eventList, std::vector<std::string> & saved) {
	std::vector <perf_event_attr> attrs;
	saved.clear ();
	for (auto & event : eventList) {
	    perf_event_attr pe  = this-> findPerfEvent (event); 
	    pe.read_format = PERF_FORMAT_ID | PERF_FORMAT_GROUP;
	    pe.disabled = 1;
		
	    if (pe.size == 0) {
		common::utils::Logger::globalInstance ().error ("Undefined perf event : ", event);
		continue;
	    } else {
		attrs.push_back (pe);
		saved.push_back (event);
	    }	    
	}
	
	return attrs;
    }

    perf_event_attr PerfEventWatcher::findPerfEvent (const std::string & name) const {
	pfm_perf_encode_arg_t arg = {};
	perf_event_attr attr = {};	
	arg.size = sizeof (arg);

	arg.attr = &attr;
	if (pfm_get_os_event_encoding (name.c_str (), PFM_PLM0 | PFM_PLM3, PFM_OS_PERF_EVENT_EXT, &arg) == PFM_SUCCESS) {
	    attr.size = sizeof (attr);
	    return attr;
	}

	attr.size = 0;
	return attr;
    }

    
    /**
     * ===================================================================================
     * ===================================================================================
     * ================================   POLLING EVENTS     =============================
     * ===================================================================================
     * ===================================================================================
     */    
    
    const std::vector<long long>& PerfEventWatcher::poll () {
	this-> _cacheSum.assign (this-> _cacheSum.size (), 0);
	
	for (int cpuId = 0 ; cpuId < this-> _fds.size () ; cpuId ++) {
	    auto size = read (this-> _fds[cpuId], this-> _cache.data (), this-> _cache.size ());
	    
	    if (size != this-> _cache.size ()) {
		common::utils::Logger::globalInstance ().error ("Failed to read perf event for cgroup : ", this-> _cgroupPath, " on cpu : ", cpuId, " read :", size, " when expecting : ", sizeof(long long) * this-> _cache.size ());
		continue;
	    }

	    auto rf = reinterpret_cast<read_format*> (this-> _cache.data ());
	    for (uint64_t i = 0 ; i < rf-> nr ; i++) {
		this-> _cacheSum[i] += rf-> values[i].value;
	    }

	    ioctl(this-> _fds[cpuId], PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
	}

	return this-> _cacheSum;
    }

    void PerfEventWatcher::poll (std::vector <common::net::Metric> & metrics) {
	common::concurrency::timer t;
	for (auto & it : metrics) {
	    it.value = 0;
	}
	
	for (int cpuId = 0 ; cpuId < this-> _fds.size () ; cpuId ++) {
	    auto size = read (this-> _fds[cpuId], this-> _cache.data (), this-> _cache.size ());
	    if (size != this-> _cache.size ()) {
		common::utils::Logger::globalInstance ().error ("Failed to read perf event for cgroup : ", this-> _cgroupPath, " on cpu : ", cpuId, " read : ", size, " when expecting : ", sizeof(long long) * this-> _cache.size ());
		continue;
	    }

	    auto rf = reinterpret_cast<read_format*> (this-> _cache.data ());
	    for (uint64_t i = 0 ; i < rf-> nr ; i++) {
		metrics[i].value += rf-> values[i].value;
	    }

	    ioctl(this-> _fds[cpuId], PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
	}	
    }

    /**
     * ===================================================================================
     * ===================================================================================
     * ================================   GETTERS     ====================================
     * ===================================================================================
     * ===================================================================================
     */    
    
    const std::vector<std::string>& PerfEventWatcher::getEventList () const {
	return this-> _eventList;
    }    

    const std::string& PerfEventWatcher::getCgroupName () const {
	return this-> _cgroupPath;
    }
    
    /**
     * ===================================================================================
     * ===================================================================================
     * ================================   CLEARING EVENTS     ============================
     * ===================================================================================
     * ===================================================================================
     */    
    
    void PerfEventWatcher::dispose () {
	if (this-> _fds.size () != 0 || this-> _toClose.size () != 0) {
	    //common::utils::Logger::globalInstance ().info ("Disposing perf watcher : ", this-> _cgroupPath);
	    
	    for (auto & it : this-> _fds) {
		ioctl(it, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);
		close (it);	    
	    }

	    for (auto & it : this-> _toClose) {
		close (it);
	    }
	    
	    this-> _toClose.clear ();       
	    this-> _fds.clear ();
	}
	
	if (this-> _cgroupFd > 0) { // if cgroupfd == -1, then there is no cgroup, so no need to clear it	    
	    close (this-> _cgroupFd);
	    this-> _cgroupFd = 0;
	}

	this-> _cacheSum.clear ();
	this-> _cache.clear ();
	this-> _eventList.clear ();
    }
    
}
