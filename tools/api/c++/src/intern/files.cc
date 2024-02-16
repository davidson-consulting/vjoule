
#include "files.hh"
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <grp.h>
#include <mntent.h>
#include <dirent.h>

namespace common::utils::api {

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


}
