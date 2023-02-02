#include <common/cgroup/cgroup.hh>
#include <common/utils/files.hh>
#include <fstream>
#include <filesystem>
#include <common/utils/_.hh>

namespace common::cgroup {

    Cgroup::Cgroup (const std::string& name) : _name (name)
    {}

    const std::string& Cgroup::getName () const {
	return this-> _name;
    }

    bool Cgroup::operator < (const Cgroup & other) const {
	return this-> _name < other._name;
    }

    void Cgroup::create () {
	if (!this-> exist ()) {
	    bool isV2 = false;
	    auto mntPoint = utils::get_cgroup_mount_point (isV2);
	    if (isV2) {
		std::filesystem::create_directories (utils::join_path (mntPoint, this-> _name));
	    }
	}
    }

    bool Cgroup::remove () {
	bool isV2 = false;
	auto mntPoint = utils::get_cgroup_mount_point (isV2);
	if (isV2) {
	    if (this-> isSlice (true, mntPoint)) {
		return false;
	    } else {
		std::filesystem::remove (utils::join_path (mntPoint, this-> _name));
	    }
	}

	return false;
    }

    bool Cgroup::exist () const {
	bool isV2 = false;
	auto mntPoint = utils::get_cgroup_mount_point (isV2);
	if (isV2) {
	    if (std::filesystem::exists (utils::join_path (utils::join_path (mntPoint, this-> _name), "controllers"))) {
		return true;
	    }
	}

	return false;
    }

    bool Cgroup::isSlice () const {
	bool isV2 = false;
	auto mntPoint = utils::get_cgroup_mount_point (isV2);
	return this-> isSlice (isV2, mntPoint);
    }

    bool Cgroup::isSlice (bool isV2, const std::string & mntPoint) const {
	if (isV2) {
	    auto cgroupPath = utils::join_path (mntPoint, this-> _name);
	    if (std::filesystem::exists (cgroupPath)) {
		for (const auto & entry : std::filesystem::directory_iterator (cgroupPath)) {
		    if (std::filesystem::is_directory (entry.path ())) {
			return true;
		    }
		}		
	    }
	}

	return false;
    }

    bool Cgroup::attach (uint64_t pid) {
	bool isV2 = false;
	auto mntPoint = utils::get_cgroup_mount_point (isV2);
	if (isV2) return this-> attachV2 (pid, mntPoint);
	
	return false;
    }

    bool Cgroup::attachV2 (uint64_t pid, const std::string & mntPoint) {
	std::ofstream s (utils::join_path (utils::join_path (mntPoint, this-> _name), "cgroup.procs"), std::ios::app);
	s << "\n" << pid;

	s.close ();
	return true;
    }
    
    void Cgroup::detachAll () {
	bool isV2 = false;
	auto mntPoint = utils::get_cgroup_mount_point (isV2);
	if (isV2) this-> detachAllV2 (mntPoint);
    }

    
    bool Cgroup::detach (uint64_t pid) {
	bool isV2 = false;
	auto mntPoint = utils::get_cgroup_mount_point (isV2);
	if (isV2) return this-> detachV2 (pid, mntPoint);
	
	return false;	
    }

    void Cgroup::detachAllV2 (const std::string & mntPoint) {
	std::stringstream ss;
	uint64_t pid;
	
	std::ifstream i (utils::join_path (utils::join_path (mntPoint, this-> _name), "cgroup.procs"));
	ss << i.rdbuf ();
	while (ss >> pid) {
	    this-> detachV2 (pid, mntPoint);
	}

	i.close ();
    }
    
    bool Cgroup::detachV2 (uint64_t pid, const std::string & mntPoint) {
	std::vector <uint64_t> attached;
	std::ofstream s (utils::join_path (mntPoint, "cgroup.procs"));
	s << "\n" << pid;
	s.close ();
	
	return true;
    }
    

}

std::ostream& operator<< (std::ostream& stream, const common::cgroup::Cgroup & group) {
    std::cout << "cg(" << group.getName () << ")";
    return stream;
}
