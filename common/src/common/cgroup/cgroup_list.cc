#include <common/cgroup/cgroup_list.hh>
#include <filesystem>
#include <fstream>
#include <common/utils/_.hh>

namespace fs = std::filesystem;
using namespace common::utils;

namespace common::cgroup {

    CgroupLister::CgroupLister () {}
    
    CgroupLister::CgroupLister (const std::vector <std::string> & rules) {
	this-> _customCgroups = rules;
    }

    std::vector <Cgroup> CgroupLister::run () const {
	std::vector <Cgroup> result;

	// recursively traverse the cgroup tree
	this-> recursiveTraverse (result, this-> _cgroupRootPath);
	return result;
    }

    /**
     * Utility function that check if the string 'fullString' finishes with 'ending' 
     * @example:
     * =====================
     * assert (hasEnding ("this is a test", "test") == true);
     * assert (hasEnding ("this is a second test", "tes") == false);
     * =====================
     */
    bool hasEnding (std::string const &fullString, std::string const &ending) {
	if (fullString.length() >= ending.length()) {
	    return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
	} else {
	    return false;
	}
    }

    void CgroupLister::recursiveTraverse (std::vector <Cgroup> & groups, const std::string & currentPath) const {
	for (auto & it : this-> _customCgroups) {
	    auto path = common::utils::join_path (currentPath, it);
	    if (common::utils::file_exists (common::utils::join_path (path, "cgroup.controllers"))) { // this is a cgroup
		this-> traverseCustomCgroup (groups, path); // we register its sub cgroups
	    }
	}	
    }

    void CgroupLister::traverseCustomCgroup (std::vector <Cgroup> & groups, const std::string & groupPath) const {
	for (const auto & entry : fs::directory_iterator(groupPath)) { // traverse the directory
	    if (fs::is_directory (entry.path ()) && common::utils::file_exists (common::utils::join_path (entry.path ().string (), "cgroup.controllers"))) { // and list all cgroup (dir with cgroup.controllers file)
		groups.push_back (Cgroup (entry.path ().string ())); // register the cgroup to watch
	    }
	}
    }
    
    void CgroupLister::traverseVMs (std::vector <Cgroup> & groups, const std::string & vmPath) const {
	for (const auto & entry : fs::directory_iterator(vmPath)) { // redondant function with traverseCustomCgroup ?
	    if (fs::is_directory (entry.path ()) && common::utils::file_exists (common::utils::join_path (entry.path ().string (), "cgroup.controllers"))) {
		groups.push_back (Cgroup (entry.path ().string ()));
	    }
	}
    }

}
