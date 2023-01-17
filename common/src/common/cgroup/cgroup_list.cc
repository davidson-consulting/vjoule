#include <common/cgroup/cgroup_list.hh>
#include <filesystem>
#include <fstream>
#include <glob.h>
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
		glob_t globbuf;
		// fetch all matching cgroups
		for(auto rule: this->_customCgroups) {
			glob(common::utils::join_path (this-> _cgroupRootPath, rule).c_str(), GLOB_ONLYDIR | GLOB_APPEND, NULL, &globbuf);
		}

		for(int i = 0; i < globbuf.gl_pathc; i++) {
			result.push_back(Cgroup(globbuf.gl_pathv[i]));
		}
		return result;
    }
}
