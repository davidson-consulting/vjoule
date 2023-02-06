#pragma once

#include <vector>
#include <string>
#include <chrono>
#include <map>
#include <iostream>
#include <ctime>

namespace vjoule {

    /**
     * Error that can be thrown by the vjoule api
     */
    struct vjoule_error {
	std::string msg;
    };
    
    /**
     * A consumption stamp is the consumption of a given entity (process, machine) at a given instant since the start
     */
    struct consumption_stamp_t {

	// The instant of the consumption 
	std::chrono::system_clock::time_point timestamp;
	
	// The CPU consumption in Joule
	double cpu;

	// The GPU consumption in Joule
	double gpu;

	// The RAM consumption in Joule
	double ram;
	
    };

    /**
     * A consumption diff is the consumption of a given entity between two consumption stamp
     */
    struct consumption_diff_t {

	// The duration in second of the diff
	float duration;

	// The CPU consumption in Joule during the duration
	double cpu;

	// The GPU consumption in Joule during the duration
	double gpu;

	// The RAM consumption in Joule during the duration
	double ram;
	
    };

    /**
     * A consumption percentage is the percentage of a consumption diff over another
     */
    struct consumption_perc_t {

	// The percentage of duration
	float duration;

	// The percentage of cpu consumption
	float cpu;

	// The percentage of gpu consumption
	float gpu;

	// The percentage of ram consumption
	float ram;

    };

    class process_group;


    /**
     * The vjoule api used to create process groups, and retreive energy consumption
     */
    class vjoule_api {
    private: 

	friend process_group;
	
	/**
	 * The list of opened group process (cgroup)
	 */
	std::map <std::string, process_group> _groups;

	// The inotify handle
	int _inotifFd = 0;

	// The watch handle
	int _inotifFdW = 0;

    private:

	vjoule_api (const vjoule_api & other);

	void operator= (const vjoule_api & other);
	
    public: 
	
	/**
	 * Create a vjoule api
	 */
	vjoule_api ();

	/**
	 * Create a process group
	 * @params: 
	 *   - name: the name of the group
	 *   - pid: the list of pids to put in the group
	 * @returns: the group process that was created
	 */
	process_group create_group (const std::string & name, const std::vector <uint64_t> & pid);
	
	/**
	 * @returns: a process group whose name is name
	 */
	process_group get_group (const std::string & name) const; 

	/**
	 * @returns: the consumption of the machine
	 */
	consumption_stamp_t get_machine_current_consumption () const;
	
	/**
	 * Clean everything started by the api
	 */
	~vjoule_api ();	


    private:

	/**
	 * Force an iteration of the vjoule service
	 */
	void forceSig () const;

	/**
	 * Wait for the service to finish computing an iteration
	 */
	void waitIteration () const;
	
    };
    
    
    class process_group {
    private:

	friend vjoule_api;

	// The context of the process group
	vjoule_api & _context;

	// The name of the cgroup
	std::string  _name;
	
	/**
	 * Create a process group with a pid list
	 */
	process_group (vjoule_api & api, const std::string & name);
		
    public: 

	/**
	 * @returns: the name of the process group
	 */
	const std::string & get_name () const;
	
	/**
	 * @returns: the consumption of the group process
	 */
	consumption_stamp_t get_current_consumption () const;

	/**
	 * @returns: true if the group process is correctly monitered and can return valid consumption_stamp_t 
	 */
	bool is_monitored () const;
	
	/**
	 * Stop monitoring the group process
	 */
	void close ();
	
    };
    
}

std::ostream & operator << (std::ostream & o, const vjoule::consumption_stamp_t & c);

std::ostream & operator << (std::ostream & o, const vjoule::consumption_diff_t & c);

std::ostream & operator << (std::ostream & o, const vjoule::consumption_perc_t & c);

vjoule::consumption_diff_t operator- (const vjoule::consumption_stamp_t & left, const vjoule::consumption_stamp_t & right);

vjoule::consumption_diff_t operator+ (const vjoule::consumption_diff_t & left, const vjoule::consumption_diff_t & right);

vjoule::consumption_diff_t operator- (const vjoule::consumption_diff_t & left, const vjoule::consumption_diff_t & right);

vjoule::consumption_perc_t operator% (const vjoule::consumption_diff_t & left, const vjoule::consumption_diff_t & right);
