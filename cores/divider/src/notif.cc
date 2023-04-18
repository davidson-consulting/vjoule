#include "notif.hh"
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <cstdio>
#include <fstream>
#include <set>

#include "notif.hh"

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

using namespace common;
using namespace common::utils;

namespace divider {

    Notifier::Notifier () {}

    void Notifier::dispose () {
		this-> _watching.clear ();

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
		std::map <std::string, int> done;
		bool v2 = false;
		std::string cgroupRoot = utils::get_cgroup_mount_point (v2);

		for (auto & it : cgroups) {
			std::string current = utils::join_path (cgroupRoot, utils::parent_directory (it));
			std::string sub = "";
			bool willInsert = false;

			while (current != cgroupRoot) {
				if (utils::file_exists (current)) {
					willInsert = true;
				} else if (utils::parent_directory (current) == cgroupRoot) {
					sub = current.substr (cgroupRoot.length () + 1);
					current = cgroupRoot;
					willInsert = true;
				}

				if (willInsert) {
					auto id = done.find (current);
					if (id == done.end ()) {
						this-> _wds.push_back (inotify_add_watch (this-> _fd, current.c_str (), IN_CREATE | IN_DELETE | IN_MODIFY));
						Watcher w {current, {sub}};
						this-> _watching.emplace (this-> _wds.back (), w);
						done.emplace (current, this-> _wds.back ());
					} else {
						this-> _watching[id-> second].inners.emplace (sub);
					}

					if (current != cgroupRoot) {
						auto n = utils::parent_directory (current);
						sub = current.substr (n.length () + 1);
						current = n;

						auto id = done.find (current);
						if (id == done.end ()) {
							this-> _wds.push_back (inotify_add_watch (this-> _fd, current.c_str (), IN_DELETE | IN_CREATE));
							Watcher w {current, {sub}};
							this-> _watching.emplace (this-> _wds.back (), w);
							done.emplace (current, this-> _wds.back ());
						} else {
							this-> _watching[id-> second].inners.emplace (sub);
						}
					}

					break;
				}

				auto next = utils::parent_directory (current);
				sub = current.substr (next.length() + 1);
				current = next;
			}
		}

		auto configDirPath = utils::parent_directory (cgroupFile);

		// add the watcher to the configuration directory
		this-> _wds.push_back (inotify_add_watch (this-> _fd, configDirPath.c_str (), IN_CREATE | IN_DELETE | IN_MODIFY));
		Watcher w {configDirPath, {"cgroups"}};
		this-> _watching.emplace (this-> _wds.back (), w);

		this-> printWatching ();
    }

    void Notifier::printWatching () const {
		std::stringstream ss;
		int i = 0;
		for (auto & it : this-> _watching) {
			if (i != 0) ss << ", ";
			ss << "[" << it.second.root << "/" << it.second.inners << "]";
			i += 1;
		}

		LOG_DEBUG ("Notifier watching ", ss.str ());
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
					if ((event-> mask & IN_CREATE) || (event-> mask & IN_DELETE) || (event-> mask & IN_MODIFY)) {
						auto sub = this-> _watching.find (event-> wd);
						if (sub != this-> _watching.end ()) {
							if (sub-> second.inners.find ("") != sub-> second.inners.end () || sub-> second.inners.find (event-> name) != sub-> second.inners.end ()) {
								LOG_DEBUG ("Notify update : ", sub-> second.root, "/", event-> name);
								this-> _onCgroupUpdate.emit ();
								return;
							}
						}
					}
				}
				i += EVENT_SIZE + event->len;
			}
		}
	}
}
