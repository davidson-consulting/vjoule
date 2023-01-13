#pragma once

#include <sensor/cgroup/cgroup.hh>
#include <common/utils/_.hh>

#include <vector>
#include <string>

namespace sensor::cgroup {

    class CgroupLister {
    private :

	// The root path of cgroup 
	std::string _cgroupRootPath = "/sys/fs/cgroup";

	// The name of machine slice (vm dir)
	std::string _machineSlice = "machine.slice";

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
	CgroupLister (const common::utils::config::dict & config);

	/**
	 * Run the cgroup lister
	 * @returns: a list of cgroups
	 */
	std::vector <Cgroup> run () const;

    private :
	
	/**
	 * Recursively traverse the cgroup directory to get all the cgroups
	 * @params:
	 *  - currentPath: the directory to traverse
	 * @returns: 
	 *  - groups: the list of groups
	 */
	void recursiveTraverse (std::vector<Cgroup>& groups, const std::string & currentPath) const;

	/**
	 * Traverse the machine.slice cgroup
	 * @params:
	 *   - vmPath: the path of the machine slice super cgroup
	 * @returns: the list of VM cgroups
	 */
	void traverseVMs (std::vector <Cgroup> & groups, const std::string& vmPath) const;

	/**
	 * Traverse a custom super cgroup slice
	 * @params:
	 *    - groupPath: the path of the slice super cgroup
	 * @returns: the list of custom cgroups
	 */
	void traverseCustomCgroup (std::vector <Cgroup> & groups, const std::string & groupPath) const;

    };
    
}
