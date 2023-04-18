#include <common/utils/files.hh>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <grp.h>
#include <mntent.h>
#include <dirent.h>

namespace common::utils {

    bool __CGROUP_V2__ = false;
    std::string __CGROUP_ROOT__ = "";
    
    
    FileError::FileError(const std::string& msg) :
		utils::CommonError (msg)
    {}

	std::string current_directory () {
		char cwd[PATH_MAX];
		if (getcwd (cwd, sizeof (cwd)) != nullptr) {
			return std::string (cwd);
		}

		return "";
	}

    bool file_exists(const std::string& name) {
		struct stat buffer;
		return (stat(name.c_str(), &buffer) == 0);
    }

	bool directory_exists (const std::string & path) {
		auto dir = opendir (path.c_str ());
		auto succ = (dir != nullptr);
		if (succ) closedir (dir);

		return succ;
	}


	bool is_empty_directory (const std::string & path) {
		if (!directory_exists (path)) return false;
		for (auto it : directory_iterator (path)) {
			return false;
		}

		return true;
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

    void own_file (const std::string & file, const std::string & groupName) {
		struct group* g = getgrnam (groupName.c_str ());
		auto paw = getpwnam (groupName.c_str ());
		auto ig = ::chown (file.c_str (), paw-> pw_uid, g-> gr_gid);
		ig = ::chown (file.c_str (), -1, g-> gr_gid);
		::chmod (file.c_str (), 0666);
	
    }
    
    bool is_parent_directory (const std::string & parent, const std::string & path) {
		if (path.length () > parent.length ()) return false;
		for (uint64_t i = 0 ; i < parent.length () ; i++) {
			if (parent [i] != path [i]) return false;
		}
		return true;
    }

	bool create_directory (const std::string & path, bool recursive) {
		auto parent = parent_directory (path);
		if (directory_exists (parent)) {
			auto code = mkdir (path.c_str (), 0775);
			return (code != -1);
		} else if (recursive) {
			if (parent != "") {
				if (!create_directory (parent, true)) return false;
			}

			return mkdir (path.c_str (), 0775) != -1;
		} else return false;
	}


    std::string get_absolute_path(const std::string& relative, const std::string& sub_dir) {
		auto pathToExe = get_current_dir_name();
		auto file_name = utils::join_path(utils::join_path(pathToExe, sub_dir), relative);
		if (utils::file_exists(file_name)) return file_name;

		file_name = utils::join_path(std::string (ANON_DIR), relative);
		if (utils::file_exists (file_name)) return file_name;
      
		if (utils::file_exists(relative)) return relative;

		throw FileError("File not found : " + file_name);
    }

	bool remove (const std::string & path) {
		if (directory_exists (path)) {
			return (rmdir (path.c_str ()) == 0);
		} else {
			return (::remove (path.c_str ()) == 0);
		}
	}

	bool create_symlink (const std::string & linkPath, const std::string & path) {
		return symlink (path.c_str (), linkPath.c_str ());
	}

    std::string get_absolute_path_if_exists(const std::string& relative, const std::string& sub_dir) {
		try {
			return get_absolute_path(relative, sub_dir);
		}
		catch (...) {
			return "";
		}
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


	DirIterator::DirIterator (DIR* dir, const std::string & path) : _dir (dir), _path (path) {
		this-> operator++ ();
	}

	void DirIterator::operator++ () {
		if (this-> _dir != nullptr) {
			this-> _curr = readdir (this-> _dir);
			if (this-> _curr == nullptr) this-> dispose ();
			else {
				if (std::string (this-> _curr-> d_name) == ".." || std::string (this-> _curr-> d_name) == ".") {
					this-> operator++ ();
				}
			}
		}
	}

	void DirIterator::dispose () {
		if (this-> _dir != nullptr) {
			closedir (this-> _dir);
			this-> _dir = nullptr;
		}
	}

	std::string DirIterator::operator* () const {
		if (this-> _curr != nullptr) {
			return utils::join_path (this-> _path, this-> _curr-> d_name);
		} else return "";
	}

	bool DirIterator::operator!= (const DirIterator & other) const {
		return this-> _dir != other._dir;
	}

	DirIterator::~DirIterator () {
		this-> dispose ();
	}


	DirIteratorCtor::DirIteratorCtor (const std::string & path) : _path (path) {}

	DirIterator DirIteratorCtor::begin () const {
		return DirIterator (opendir (this-> _path.c_str ()), this-> _path);
	}

	DirIterator DirIteratorCtor::end () const {
		return DirIterator (nullptr, "");
	}


	DirIteratorCtor directory_iterator (const std::string & path) {
		return DirIteratorCtor (path);
	}


}


