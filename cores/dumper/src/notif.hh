#pragma once

#include <common/concurrency/_.hh>
#include <common/_.hh>
#include <vector>

namespace dumper {

    /**
     * This class notify when a modification is made to a file that modifies the context of the divider
     */
    class Notifier {
    private :

	// Event triggered when there is an update in cgroup file system
	common::concurrency::signal<> _onCgroupUpdate;

	common::concurrency::signal<> _onConfigUpdate;
	
	// The file handle
	int _fd = 0;

	// the watch handle
	std::vector <int> _wds;

	// The watching thread
	common::concurrency::thread _th;

	// The root directory of cgroup
	std::string _cgroupRootPath = "/sys/fs/cgroup";

	// The path of the configuration file
	std::string _configPath;
    
    public :

	/**
	 * Create an empty cgroup notifier (deactivated at first)
	 */
	Notifier ();

	/**
	 * Start the thread of the cgroup notifier
	 */
	void start ();

	/**
	 * Start the cgroup notifier in the current thread
	 * @warning: infinite loop
	 */
	void startSync ();
	
	/**
	 * Configure the notifier to wait the updates of "cgroupPath" and "configDirPath"	 
	 */
	void configure (const std::string & cgroupFile, const std::vector <std::string> & cgroups);
	
	/**
	 * Kill the running notifier
	 */
	void kill ();

	/**
	 * @returns: the signal emitted when there is a modification to a watched file
	 */
	common::concurrency::signal<>& onUpdate () ;
	
    private :

	/**
	 * Close inotify
	 */
	void dispose ();

	/**
	 * Loop waiting for cgroup of config dir changes
	 */
	void waitLoop (common::concurrency::thread);

	/**
	 * Traverse the cgroup sub directories to attach watchers
	 */
	void traverseCgroupDirectories (const std::string & path);
	
    };
  
}
