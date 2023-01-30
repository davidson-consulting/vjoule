#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <cstdio>
#include <string.h>
#include <ctime>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <map>

#define VJOULE_PATH "/etc/vjoule/results/"
#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

namespace fs = std::filesystem;

typedef std::map <std::string, std::vector <double> > conso_hist;

/**
 * Stream operator to print conso_hist to std::cout
 */
std::ostream& operator<< (std::ostream & s, const conso_hist& hist) {
    s << "{" << std::endl;
    for (auto & it : hist) {
	if (it.first == "") {
	    s << "\tSystem = [";
	} else {
	    s << "\t" << it.first << " = [";
	}
	int i = 0;
	double last = 0.0;
	if (it.second.size () >= 2) {
	    s << (it.second.back () - it.second[it.second.size () - 2]) << "W";
	}
	s << "]" << std::endl;
    }
    s << "}";

    return s;
}

/**
 * Create a inotify watcher waiting for updates from the service
 * This is the correct way of waiting results, as it is a passive wait
 * @warning: this function returns only when a new result is written by the service
 */
void waitServiceIteration () {
    fs::path resultPath = fs::path (VJOULE_PATH) / "cpu";
    if (!fs::exists (resultPath)) {
	throw std::runtime_error ("Divider service is not running");
    }
    
    auto fd = inotify_init (); // create a inotify file descriptor
    
    // attach it to modification in the formula directory
    // Unfortunately it seems that inotify cannot be attached directly to a file
    auto wd = inotify_add_watch (fd, VJOULE_PATH, IN_MODIFY); 
    

    char buffer[EVENT_BUF_LEN];
    while (true) {	
	auto len = read (fd, buffer, EVENT_BUF_LEN); // wait for events
	if (len == 0) { // file descriptor was closed without notice ?
	    throw std::runtime_error ("waiting inotify");
	}

	int i = 0;
	while (i < len) { // there can be multiple events to read
	    struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];

	    // if there is an event, that event is a modification and it was of the formula.signal
	    if (event-> len != 0 && event-> mask & IN_MODIFY && strcmp (event-> name, "cpu") == 0) {
		inotify_rm_watch (fd, wd); // close the watched
		close (fd); // close inotify
		return; // there was an event we can stop waiting
	    }
	    
	    i += EVENT_SIZE + event->len;
	}	
    }    
}

/**
 * Read the consumption of a cgroup and updates the history values
 * @returns: 
 *    - cpuConso: insert a new value
 *    - ramConso: insert a new value
 * @params:
 *    - cgroupPath: the path of the cgroup to read
 */
void readConsumption (conso_hist & cpuConso, conso_hist & ramConso, conso_hist & gpuConso, const fs::path & cgroupPath) {
    auto ramPath = cgroupPath / "ram";
    auto cpuPath = cgroupPath / "cpu";
    auto gpuPath = cgroupPath / "gpu";

    auto cgroupName = fs::path (cgroupPath.string ().substr (strlen (VJOULE_PATH)));

    double ram = 0.0, cpu = 0.0, gpu = 0.0;
    
    if (fs::exists (ramPath)) {
	std::ifstream ramF (ramPath);	
	ramF >> ram;
	ramF.close ();
    }

    if (fs::exists (cpuPath)) {
	std::ifstream cpuF (cpuPath);	
	cpuF >> cpu;
	cpuF.close ();
    }

    if (fs::exists (gpuPath)) {
	std::ifstream gpuF (cpuPath);	
	gpuF >> gpu;
	gpuF.close ();
    }

    
    auto cfnd = cpuConso.find (cgroupName);
    if (cfnd != cpuConso.end ()) {
	cfnd-> second.push_back (cpu); 
    } else {
	cpuConso.emplace (cgroupName, std::vector<double> ({ cpu })); // insert a new entry in the results
    }

    auto rfnd = ramConso.find (cgroupName);
    if (rfnd != ramConso.end ()) {
	rfnd-> second.push_back (ram); 
    } else {
	ramConso.emplace (cgroupName, std::vector<double> ({ ram })); // insert a new entry in the results
    }

    auto gfnd = gpuConso.find (cgroupName);
    if (gfnd != gpuConso.end ()) {
	gfnd-> second.push_back (gpu); 
    } else {
	gpuConso.emplace (cgroupName, std::vector<double> ({ gpu })); // insert a new entry in the results
    }
}


/**
 * Read the tree of cgroups and updates the read consumption
 * @returns:
 *   - cpuConso: add an entry for every cgroup 
 *   - ramConso: add an entry for every cgroup
 * @params: 
 *    - i: the current iteration
 *    - current: the path to traverse recursively
 */
void readCgroupTree (conso_hist & cpuConso, conso_hist & ramConso, conso_hist & gpuConso, const fs::path& current = fs::path (VJOULE_PATH)) {
    if (fs::exists (current / "cpu")) {
	readConsumption (cpuConso, ramConso, gpuConso, current);
    }
    
    for (const auto & entry : fs::directory_iterator (current)) {
	if (fs::is_directory (entry.path ())) {
	    readCgroupTree (cpuConso, ramConso, gpuConso, entry.path ());
	}
    }
}

int main () {
    conso_hist cpuConso, ramConso, gpuConso;
    std::vector <time_t> timestamps;
    
    while (true) {
	waitServiceIteration (); // wait for a formula update
	time_t now;
	time (&now);
	timestamps.push_back (now);

	char buf[sizeof "2011-10-08 07:07:09"];
	strftime(buf, sizeof buf, "%F %T", gmtime(&now));
	
	readCgroupTree (cpuConso, ramConso, gpuConso); // read the cgroup values
	std::cout << "===================  " << buf << "  ======================" << std::endl;
	std::cout << "CPU : " << cpuConso << std::endl;
	std::cout << "RAM : " << ramConso << std::endl;
	std::cout << "GPU : " << gpuConso << std::endl;
    }
    
}
