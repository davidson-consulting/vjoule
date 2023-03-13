#include <common/cgroup/cgroup_list.hh>
#include <fstream>
#include <glob.h>
#include <common/utils/_.hh>

using namespace common::utils;

namespace common::cgroup {

    CgroupLister::CgroupLister () {
        bool v2 = false;
        this-> _cgroupRootPath = utils::get_cgroup_mount_point (v2);
    }
    
    CgroupLister::CgroupLister (const std::vector <std::string> & rules) {
        this-> _customCgroups = rules;
        bool v2 = false;
        this-> _cgroupRootPath = utils::get_cgroup_mount_point (v2);
    }

    std::set <Cgroup> CgroupLister::run () const {
        std::set <Cgroup> list;
        list.emplace (Cgroup (""));
	
        if (this-> _customCgroups.size () != 0) {
            // fetch all matching cgroups
            for(auto rule: this-> _customCgroups) {
                glob_t globbuf;
                int i = glob (common::utils::join_path (this-> _cgroupRootPath, rule).c_str (), GLOB_ONLYDIR, NULL, &globbuf);
                if (i != GLOB_NOMATCH) {
                    for(int i = 0; i < globbuf.gl_pathc; i++) {
                        list.emplace (Cgroup (globbuf.gl_pathv[i]));
                    }
                }
                globfree (&globbuf);
            }
	    
        }
	
        return list;
    }
}
