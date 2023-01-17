#pragma once

#include <common/cgroup/cgroup.hh>
#include <common/utils/_.hh>

#include <vector>
#include <set>
#include <string>

namespace common::cgroup {

    class CgroupLister {
    private :

	// The root path of cgroup 
	std::string _cgroupRootPath = "/sys/fs/cgroup";

	// The list of custom cgroup to take into account
	std::vector <std::string> _customCgroups;

	bool _considerVMs = true;
	
    public :

	/**
	 * Create an empty cgroup lister
	 */
	CgroupLister ();
	
	/**
	 * Create a cgroup lister with a given configuration
	 */
	CgroupLister (const std::vector <std::string> & rules);

	/**
	 * Run the cgroup lister
	 * @returns: a list of cgroups
	 */
	std::set <Cgroup> run () const;
    };
    
}
