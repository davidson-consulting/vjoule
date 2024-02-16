#pragma once
#include <sys/stat.h>
#include <vector>
#include <string>

#define VJOULE_DIR "/etc/vjoule/"

namespace common::utils::api {
		
    /**
     * @return: true if the file at path `name` exists, false otherwise
     */
    bool file_exists(const std::string& name);

    /**
     * Joins the path in one single path
     */
    std::string join_path(const std::string& begin, const std::string& end);

    /**
     * @returns: the parent directory of the path
     */
    std::string parent_directory(const std::string& path);

    /**
     * @returns: true of parent is a parent directory of path
     */
    bool is_parent_directory (const std::string & parent, const std::string & path);

}
