#pragma once
#include <sys/stat.h>
#include <common/utils/error.hh>
#include <vector>
#include <dirent.h>

#define VJOULE_DIR "/etc/vjoule/"

namespace common::utils {

       
    class FileError : public utils::CommonError {
        public:
            FileError(const std::string& msg);
    };
		
    /**
     * @return: true if the file at path `name` exists, false otherwise
     */
    bool file_exists(const std::string& name);

    /**
     * @returns: the path of the current working directory
     * */
    std::string current_directory ();

    /**
     * Joins the path in one single path
     */
    std::string join_path(const std::string& begin, const std::string& end);

    /**
     * Change the ownership of a file, and permission to 0666
     */
    void own_file (const std::string & filepath, const std::string & group);

    
    /**
     * @returns: the parent directory of the path
     */
    std::string parent_directory(const std::string& path);

    /**
     * @returns: true of parent is a parent directory of path
     */
    bool is_parent_directory (const std::string & parent, const std::string & path);

    /**
     * @returns: true iif the path is a directory
     * */
    bool directory_exists (const std::string & path);

    /*
     * @returns: true if 'path' points to a directory and it is empty
     * */
    bool is_empty_directory (const std::string & path);

    /*
     * Create a directory
     * @params:
     *    - path: the path of the directory to create
     *    - recursive: if true create parent directories that does not exist
     * @returns: false if the creation failed
     */
    bool create_directory (const std::string & path, bool recursive);

    /*
     * Remove the fs entry at 'path'
     * @params:
     *    - path: the path of the entry to remove
     * @returns: true iif the entry was removed
     * */
    bool remove (const std::string & path);


    /**
     * Create a link at path 'linkPath' pointing to 'path'
     * */
    bool create_symlink (const std::string & linkPath, const std::string & path);

    /**
     * @return: the absolute path of the file
     * @info: the file will be search in INCLUDE_DIR and ADDONS
     * @throw: file_error, if the file does not exists
     * @params:
     *    - relative: the relative name of the file
     *    - sub_dir: the sub directory in the include_dir
     */
    std::string get_absolute_path(const std::string& relative, const std::string& sub_dir = "res/");

    /**
     * @returns: the absolute path of the file (if it exists), return "" if the file is not found
     * @info: the function calls get_absolute_path, but catch the error
     * @nothrow
     * @params:
     *    - relative: the relative name of the file
     *    - sub_dir: the sub directory in the include_dir
     */
    std::string get_absolute_path_if_exists(const std::string& relative, const std::string& sub_dir = "");

    /**
     * @returns: the type of mount of a given path
     */
    std::string get_mount_type (const std::string & path);

    /**
     * @returns: the list of path that are mounted with type type
     */
    std::vector <std::string> get_mount_loc (const std::string & type);

    /**
     * @returns: the mount point of cgroup dir
     */
    std::string get_cgroup_mount_point (bool &isV2);


    class DirIterator {

        DIR * _dir;
        struct dirent* _curr;
        std::string _path;

        public:

            DirIterator (DIR* dir, const std::string & path);
            void operator++ ();
            bool operator!= (const DirIterator&) const;
            std::string operator* () const;

            void dispose ();

            ~DirIterator ();
    };

    class DirIteratorCtor {

        std::string _path;

        public:

            DirIteratorCtor (const std::string & path);

            DirIterator begin () const;

            DirIterator end () const;

    };



    DirIteratorCtor directory_iterator (const std::string & path);


}
