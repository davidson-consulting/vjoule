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

#define VJOULE_PATH "/etc/vjoule/simple_formula/"
#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

namespace fs = std::filesystem;

typedef std::map <std::string, std::map <int, double> > conso_hist;

/**
 * Stream operator to print conso_hist to std::cout
 */
std::ostream& operator<< (std::ostream & s, const conso_hist& hist) {
    s << "{" << std::endl;
    for (auto & it : hist) {
	s << "\t" << it.first << "= [";
	int i = 0;
	double last = 0.0;
	for (auto & jt : it.second) {
	    if (i != 0) s << ", ";
	    s << jt.first << " : " << (jt.second - last); // consumption of formula is cumulative, so to have immediate consumption we need to subtract the last value
	    last = jt.second;
	    i += 1;
	}
	s << "]" << std::endl;
    }
    s << "}";

    return s;
}

/**
 * Create a inotify watcher waiting for updates from the formula
 * This is the correct way of waiting formula, as it is a passive wait
 * @warning: this function returns only when the signal was emitted by the formula
 */
void waitSignalFormula () {
    fs::path signalPath = fs::path (VJOULE_PATH) / "formula.signal";
    if (!fs::exists (signalPath)) {
	throw std::runtime_error ("Formula is not running");
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
	    if (event-> len != 0 && event-> mask & IN_MODIFY && strcmp (event-> name, "formula.signal") == 0) {
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
void readConsumption (int i, conso_hist & cpuConso, conso_hist & ramConso, const fs::path & cgroupPath) {
    auto ramPath = cgroupPath / "memory";
    auto cpuPath = cgroupPath / "package";

    auto cgroupName = fs::path (cgroupPath.string ().substr (strlen (VJOULE_PATH)));

    double ram = 0.0, cpu = 0.0;
    
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

    auto cfnd = cpuConso.find (cgroupName);
    if (cfnd != cpuConso.end ()) {
	cfnd-> second.emplace (i, cpu); 
    } else {
	cpuConso.emplace (cgroupName, std::map <int, double> ({ {i, cpu} })); // insert a new entry in the results
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
void readCgroupTree (int i, conso_hist & cpuConso, conso_hist & ramConso, const fs::path& current = fs::path (VJOULE_PATH)) {
    if (fs::exists (current / "package")) {
	readConsumption (i, cpuConso, ramConso, current);
    }
    
    for (const auto & entry : fs::directory_iterator (current)) {
	if (fs::is_directory (entry.path ())) {
	    readCgroupTree (i, cpuConso, ramConso, entry.path ());
	}
    }
}

int main () {
    conso_hist cpuConso, ramConso;
    std::vector <time_t> timestamps;
    int nb_read = 100;
    
    for (int i = 0 ; i < nb_read ; i++) {
	waitSignalFormula (); // wait for a formula update
	time_t now;
	time (&now);
	timestamps.push_back (now);

	char buf[sizeof "2011-10-08 07:07:09"];
	strftime(buf, sizeof buf, "%F %T", gmtime(&now));
	
	readCgroupTree (i, cpuConso, ramConso); // read the cgroup values
	std::cout << buf << std::endl << cpuConso << std::endl << ramConso << std::endl;
    }
    
}
