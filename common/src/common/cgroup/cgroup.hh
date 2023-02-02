#pragma once
#include <iostream>

namespace common::cgroup {

    class Cgroup {
    private : 

	// The name of the cgroup
	std::string _name;

    public :
	
	/**
	 * Create a cgroup from a name
	 */
	Cgroup (const std::string & name);

	/**
	 * @returns: the name of the cgroup
	 */
	const std::string& getName () const;

	/**
	 * Create the cgroup on the system
	 */
	void create ();

	/**
	 * Remove the cgroup from the system
	 * @returns: true if succeded
	 */
	bool remove () ;

	/**
	 * Attach a pid to the cgroup
	 */
	bool attach (uint64_t pid);

	/**
	 * Detach the pid from the cgroup
	 * @returns: true if succeded (false means the process is attached to the cgroup)
	 */
	bool detach (uint64_t pid);

	/**
	 * Detach all the pids attached to the cgroup
	 */
	void detachAll ();
	
	/**
	 * @returns: true if the cgroup exist
	 */
	bool exist () const;

	/**
	 * @returns: true if the cgroup is a slice
	 */
	bool isSlice () const;

	
	bool operator< (const Cgroup & other) const;
	
    private:

	/**
	 * @returns: true if the cgroup is a slice
	 */
	bool isSlice (bool isV2, const std::string & mntPoint) const;

	/**
	 * attach a pid to cgroup in v2
	 */
	bool attachV2 (uint64_t pid, const std::string & mntPoint);

	/**
	 * detach a pid to cgroup in v2
	 */
	bool detachV2 (uint64_t pid, const std::string & mntPoint);

	/**
	 * detach all v2
	 */
	void detachAllV2 (const std::string & mntPoint);

	
    };

}

std::ostream& operator<< (std::ostream& stream, const common::cgroup::Cgroup & group);
