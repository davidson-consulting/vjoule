#include "notif.hh"
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <set>

#include "notif.hh"

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

namespace fs = std::filesystem;
using namespace common;
using namespace common::utils;

namespace divider {

    Notifier::Notifier () {}
    
    void Notifier::dispose () {
	for (auto & it : this-> _wds) {
	    inotify_rm_watch (this-> _fd, it);
	}

	this-> _wds.clear ();
	if (this-> _fd != 0) {
	    close (this-> _fd);
	    this-> _fd = 0;
	}
    }

    void Notifier::configure (const std::string & cgroupFile, const std::vector <std::string> & cgroups) {
	this-> dispose ();
	
	this-> _fd = inotify_init ();
	std::set <std::string> watched;
	for (auto & it : cgroups) {
	    auto parent = utils::join_path ("/sys/fs/cgroup", utils::parent_directory (it));
	    if (parent != "/sys/fs/cgroup") {
		watched.emplace (parent);
	    }
	}

	
	for (auto & it : watched) {
	    this-> _wds.push_back (inotify_add_watch (this-> _fd, it.c_str (), IN_CREATE | IN_DELETE | IN_MODIFY));
	}
	
	auto configDirPath = utils::parent_directory (cgroupFile);	
	
	// add the watcher to the configuration directory
	this-> _wds.push_back (inotify_add_watch (this-> _fd, configDirPath.c_str (), IN_CREATE | IN_DELETE | IN_MODIFY));	
    }
    
    void Notifier::start () {
	concurrency::spawn (this, &Notifier::waitLoop);
    }

    void Notifier::kill () {
	if (this-> _th != 0) {
	    concurrency::kill (this-> _th);	
	    this-> _th = 0;
	}

	this-> dispose ();
    }

    common::concurrency::signal<>& Notifier::onUpdate () {
	return this-> _onCgroupUpdate;
    }

    void Notifier::waitLoop (common::concurrency::thread) {
	char buffer[EVENT_BUF_LEN];
	while (true) {
	    auto len = read (this-> _fd, buffer, EVENT_BUF_LEN);
	    if (len == 0) {
		throw std::runtime_error ("reading cgroup notif");
	    }

	    int i = 0;
	    while (i < len) {
		struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];      
		if (event-> len != 0) { // there is an event to read
		    if (event-> mask & IN_CREATE || event-> mask & IN_DELETE | event-> mask & IN_MODIFY) {
			// we don't want to emit for the modification of port, or log files. the only interesting file in config is config.toml
			if (event-> wd == this-> _wds.back () && std::string (event-> name) == "cgroups") { 
			    this-> _onCgroupUpdate.emit ();
			    return;
			} else if (event-> wd != this-> _wds.back ()) { // cgroup modification
			    LOG_DEBUG ("Notify update : ", event-> name);
			    this-> _onCgroupUpdate.emit ();
			    return;
			}
		    }
		}
		i += EVENT_SIZE + event->len;
	    }
	}
    }
}
