#include <iostream>
#include <sys/inotify.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>

#include <watcher.hh>

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

namespace tools::vjoule {
  

  Watcher::Watcher(std::string path, std::string file): _path(path), _file(file) {}

  void Watcher::wait() {
    auto fd = inotify_init();
    auto wd = inotify_add_watch(fd, this-> _path.c_str(), IN_CREATE | IN_MODIFY);
    char buffer[EVENT_BUF_LEN];

    while (true) {
      auto len = read (fd, buffer, EVENT_BUF_LEN);
      if (len == 0) { // file descriptor was closed without notice ?
	throw std::runtime_error ("waiting inotify");
      }

      int i = 0;
      while (i < len) {
	struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
	
	if (event-> len != 0 && event-> mask & IN_MODIFY && strcmp (event-> name, this-> _file.c_str()) == 0) {
	  inotify_rm_watch (fd, wd);
	  close (fd);
	  return;
	}

	i += EVENT_SIZE + event->len;
      }
    }
  }
}
