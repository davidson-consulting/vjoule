#include "vjoule_api.hh"
#include "intern/files.hh"
#include "intern/signal.hh"

#include <sstream>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <sys/inotify.h>

using namespace common;



#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )


concurrency::signal<> __exit_signal__;

namespace vjoule {
      
    vjoule_api::vjoule_api () {	
	__exit_signal__.connect (this, &vjoule_api::on_exit);	
	
	auto sys = system ("systemctl is-active --quiet vjoule_service.service");
	if (sys != 0) {
	    throw vjoule::vjoule_error ("vJoule service is not started. sudo systemctl start vjoule_service.service");
	}

	this-> _inotifFd = inotify_init ();
	this-> _inotifFdW = inotify_add_watch (this-> _inotifFd, utils::join_path (VJOULE_DIR, "results").c_str (), IN_MODIFY | IN_CREATE);	
	
	auto create = system (("vjoule_cgutils add vjoule_api.slice/this_" + std::to_string (getpid ())).c_str ());
	if (create != 0) {
	    throw vjoule::vjoule_error ("failed to create cgroup. make sure the current user is in group 'vjoule'.");
	}

	auto attach = system (("vjoule_cgutils attach vjoule_api.slice/this_" + std::to_string (getpid ()) + " " + std::to_string (getpid ())).c_str ());
	if (attach != 0) {
	    throw vjoule::vjoule_error ("failed to attach pid to cgroup. make sure the current user is in group 'vjoule'.");
	}

	this-> _groups.emplace ("this", process_group (*this, "this_" + std::to_string (getpid ())));
	this-> _groups.emplace ("this_" + std::to_string (getpid ()), process_group (*this, "this_" + std::to_string (getpid ())));

	std::ifstream i (utils::join_path (VJOULE_DIR, "cgroups"), std::ios::app);

	bool found = false;
	std::string line;
	while (std::getline(i, line)) {
	    if (line == "vjoule_api.slice/*") {
		found = true;
		break;
	    }
	}

	if (!found) {
	    std::ofstream f (utils::join_path (VJOULE_DIR, "cgroups"), std::ios::app);
	    f << "vjoule_api.slice/*" << std::endl;
	    f.close ();
	}

	this-> force_sig ();
	
    }

    process_group vjoule_api::create_group (const std::string & name, const std::vector <uint64_t> & pidList) {
	std::stringstream ss;
	ss << "vjoule_cgutils attach " << name << " ";
	for (auto & p : pidList) {
	    ss << p << " " ;
	}

	auto attach = system (ss.str ().c_str ());
	if (attach != 0) {
	    throw vjoule::vjoule_error ("failed to attach pid to cgroup. make sure the current user is in group 'vjoule'.");
	}
	
	this-> _groups.emplace (name, process_group (*this, name));	
	return process_group (*this, name);
    }    

    process_group vjoule_api::get_group (const std::string & name) const {
	auto it = this-> _groups.find (name);
	if (it != this->_groups.end ()) return it-> second;

	throw vjoule::vjoule_error ("no process group named '" + name + "' is monitored.");
    }
    
    consumption_stamp_t vjoule_api::get_machine_current_consumption () const {
	this-> force_sig ();
	consumption_stamp_t t (std::chrono::system_clock::now (), 0, 0, 0);
	auto cpu = utils::join_path (VJOULE_DIR, "results/cpu");
	auto gpu = utils::join_path (VJOULE_DIR, "results/gpu");
	auto ram = utils::join_path (VJOULE_DIR, "results/ram");
	{
	    std::ifstream f (cpu);
	    if (f.is_open ()) {
		f >> t.cpu;
	    }
	}
	
	{
	    std::ifstream f (gpu);
	    if (f.is_open ()) {
		f >> t.gpu;
	    }
	}
	
	{
	    std::ifstream f (ram);
	    if (f.is_open ()) {
		f >> t.ram;
	    }
	}

	return t;	
    }

    void vjoule_api::force_sig () const {
	for (uint64_t i = 0 ; i < 2 ; i++) {
	    FILE * f = fopen (utils::join_path (VJOULE_DIR, "signal").c_str (), "w");
	    if (f == nullptr) throw vjoule::vjoule_error ("failed to signal service.");
	    fprintf (f, "%d\n", 1);
	    fflush (f);
	    fclose (f);
	    
	    usleep (10000);
	    this-> wait_iteration ();
	}
    }

    void vjoule_api::wait_iteration () const {
	char buffer[EVENT_BUF_LEN];
	while (true) {
	    auto len = read (this-> _inotifFd, buffer, EVENT_BUF_LEN);
	    if (len == 0) {
		throw std::runtime_error ("reading cgroup notif");
	    }

	    int i = 0;
	    while (i < len) {
		struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];      
		if (event-> len != 0) { // there is an event to read
		    if (event-> mask & IN_CREATE || event-> mask & IN_DELETE | event-> mask & IN_MODIFY) {
			return;
		    }
		}
		i += EVENT_SIZE + event->len;
	    }
	}   
    }

    void vjoule_api::on_exit () {
	for (auto & it : this-> _groups) {
	    it.second.close ();
	}
       

	if (this-> _inotifFdW != 0) {
	    inotify_rm_watch (this-> _inotifFd, this-> _inotifFdW);
	    close (this-> _inotifFd);
	    this-> _inotifFd = 0;
	    this-> _inotifFdW = 0;
	}
    }
    
    
    vjoule_api::~vjoule_api () {
	this-> on_exit ();
    }    

    
    process_group::process_group (vjoule_api & api, const std::string & name) :
	_name (name), _context (api)
    {}

    const std::string & process_group::get_name () const {
	return this-> _name;
    }
    

    bool process_group::is_monitored () const {
	auto fs = utils::join_path (VJOULE_DIR, "results/vjoule_api.slice/" + this-> _name + "/cpu");
	return utils::file_exists (fs);
    }

    consumption_stamp_t process_group::get_current_consumption () const {
	this-> _context.force_sig ();
	
	consumption_stamp_t t (std::chrono::system_clock::now (), 0, 0, 0);	
	auto cpu = utils::join_path (utils::join_path (utils::join_path (VJOULE_DIR, "results/vjoule_api.slice/"), this-> _name), "cpu");
	auto gpu = utils::join_path (utils::join_path (utils::join_path (VJOULE_DIR, "results/vjoule_api.slice/"), this-> _name), "gpu");
	auto ram = utils::join_path (utils::join_path (utils::join_path (VJOULE_DIR, "results/vjoule_api.slice/"), this-> _name), "ram");
		
	{
	    std::ifstream f (cpu);
	    if (f.is_open ()) {
		f >> t.cpu;
	    }
	}
	
	{
	    std::ifstream f (gpu);
	    if (f.is_open ()) {
		f >> t.gpu;
	    }
	}
	
	{
	    std::ifstream f (ram);
	    if (f.is_open ()) {
		f >> t.ram;
	    }
	}

	return t;
    }    

    
    void process_group::close () {
	auto del = system (("vjoule_cgutils del vjoule_api.slice/" + this-> _name).c_str ());
	if (del != 0) {
	    throw vjoule::vjoule_error ("failed to delete cgroup. make sure the current user is in group 'vjoule'.");
	}
    }
    
}


std::ostream & operator << (std::ostream & o, const vjoule::consumption_stamp_t & c) {
    char buffer[255];
    snprintf (buffer, 255, "stamp (%ld, cpu: %.2fJ, ram: %.2fJ, gpu: %.2fJ)",
	      c.timestamp.time_since_epoch ().count (),
	      c.cpu, 
	      c.ram,
	      c.gpu);
    
    o << buffer;
    return o;
}

std::ostream & operator << (std::ostream & o, const vjoule::consumption_diff_t & c) {
    char buffer[255];
    snprintf (buffer, 255, "diff (time: %.2fs, cpu: %.2fJ, ram: %.2fJ, gpu: %.2fJ)",
	      c.duration,
	      c.cpu, 
	      c.ram,
	      c.gpu);
    
    o << buffer;

    return o;
}

std::ostream & operator << (std::ostream & o, const vjoule::consumption_perc_t & c) {
    char buffer[255];
    snprintf (buffer, 255, "perc (time: %.2f%c, cpu: %.2f%c, ram: %.2f%c, gpu: %.2f%c)",
	      c.duration, '%',
	      c.cpu, '%',
	      c.ram, '%',
	      c.gpu, '%');
    
    o << buffer;

    return o;
}

vjoule::consumption_diff_t operator- (const vjoule::consumption_stamp_t & left, const vjoule::consumption_stamp_t & right) {
    std::chrono::duration<double> diff = left.timestamp - right.timestamp;

    double c = left.cpu - right.cpu;
    double g = left.gpu - right.gpu;
    double r = left.ram - right.ram;

    return vjoule::consumption_diff_t (diff.count (), c, g, r);
}

vjoule::consumption_diff_t operator+ (const vjoule::consumption_diff_t & left, const vjoule::consumption_diff_t & right) {
    float t = left.duration + right.duration;
    double c = left.cpu + right.cpu;
    double g = left.gpu + right.gpu;
    double r = left.ram + right.ram;

    return vjoule::consumption_diff_t (t, c, g, r);
}

vjoule::consumption_diff_t operator- (const vjoule::consumption_diff_t & left, const vjoule::consumption_diff_t & right) {
    float t = left.duration - right.duration;
    double c = left.cpu - right.cpu;
    double g = left.gpu - right.gpu;
    double r = left.ram - right.ram;

    return vjoule::consumption_diff_t (t, c, g, r);
}


vjoule::consumption_perc_t operator% (const vjoule::consumption_diff_t & left, const vjoule::consumption_diff_t & right) {

    float t = left.duration / right.duration * 100;
    float c = left.cpu / right.cpu * 100;
    float g = left.gpu / right.gpu * 100;
    float r = left.ram / right.ram * 100;

    return vjoule::consumption_perc_t (t, c, g, r);
}
