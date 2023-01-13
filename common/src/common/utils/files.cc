#include <common/utils/files.hh>
#include <unistd.h>

namespace common::utils {

  FileError::FileError(const std::string& msg) :
    utils::CommonError (msg)
  {}

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

  std::string get_absolute_path(const std::string& relative, const std::string& sub_dir) {
      auto pathToExe = get_current_dir_name();
      auto file_name = utils::join_path(utils::join_path(pathToExe, sub_dir), relative);
      if (utils::file_exists(file_name)) return file_name;

      file_name = utils::join_path(std::string (VJOULE_DIR), relative);      
      if (utils::file_exists (file_name)) return file_name;      
      
      if (utils::file_exists(relative)) return relative;

      throw FileError("File not found : " + file_name);
  }

  std::string get_absolute_path_if_exists(const std::string& relative, const std::string& sub_dir) {
    try {
      return get_absolute_path(relative, sub_dir);
    }
    catch (...) {
      return "";
    }
  }

}


