#include "files.hh"
#include <unistd.h>

#include <mntent.h>

namespace common::utils {

    bool __CGROUP_V2__ = false;
    std::string __CGROUP_ROOT__ = "";
    
    
    bool file_exists(const std::string& name) {
	struct stat buffer;
	return (stat(name.c_str(), &buffer) == 0);
    }

    std::string join_path(const std::string& begin, const std::string& end) {
	if (begin.length() != 0 && end.length() != 0) {
	    bool need = true;
	    if (begin[begin.length() - 1] == '/') need = false;
	    if (end[0] == '/') {
		if (need) return begin + end;
		else return begin + end.substr(1);
	    }
	    else {
		if (need) return begin + "/" + end;
		else return begin + end;
	    }
	}
	else {
	    return begin + end;
	}
    }

    std::string parent_directory(const std::string& path) {
	auto i = path.find_last_of('/');
	if (i != std::string::npos) {
	    return path.substr(0, path.find_last_of('/'));
	}
	else {
	    return "";
	}
    }

    bool is_parent_directory (const std::string & parent, const std::string & path) {
	if (path.length () > parent.length ()) return false;
	for (uint64_t i = 0 ; i < parent.length () ; i++) {
	    if (parent [i] != path [i]) return false;
	}
	return true;
    }
 
    std::string get_mount_type (const std::string & path) {
	struct mntent *ent;
	FILE *aFile;

	aFile = setmntent("/proc/mounts", "r");
	if (aFile == NULL) {
	    perror("setmntent");
	    exit(1);
	}
	std::string result = "";
	while (NULL != (ent = getmntent(aFile))) {
	    if (is_parent_directory (ent-> mnt_dir, path)) {
		result = std::string (ent-> mnt_fsname);
		break;
	    }	    
	}
	
	endmntent(aFile);
	return result;
    }

    std::vector <std::string> get_mount_loc (const std::string & type) {
	struct mntent *ent;
	FILE *aFile;

	aFile = setmntent("/proc/mounts", "r");
	if (aFile == NULL) {
	    perror("setmntent");
	    exit(1);
	}

	std::vector <std::string> result;
	while (NULL != (ent = getmntent(aFile))) {
	    if (type == std::string (ent-> mnt_fsname)) {
		result.push_back (ent-> mnt_dir);
	    }
	}
	
	endmntent(aFile);

	return result;
    }


    std::string get_cgroup_mount_point (bool & isV2) {
	if (__CGROUP_ROOT__ != "") {
	    isV2 = __CGROUP_V2__;
	    return __CGROUP_ROOT__;
	}
	
	auto res = get_mount_loc ("cgroup2");
	if (res.size () != 0) {
	    isV2 = true;
	    __CGROUP_V2__ = true;
	    __CGROUP_ROOT__ = res[0];

	    return res[0];
	}

	res = get_mount_loc ("cgroup");
	if (res.size () != 0) {
	    __CGROUP_ROOT__ = res[0];
	    return res[0];
	}

	return "";
    }
    
}


