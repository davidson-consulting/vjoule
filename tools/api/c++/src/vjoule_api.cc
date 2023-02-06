#include "vjoule_api.hh"
#include "intern/cgroup.hh"
#include "intern/files.hh"
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <sys/inotify.h>

using namespace common;



#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )


namespace vjoule {
      
    vjoule_api::vjoule_api () {
	auto sys = system ("systemctl is-active --quiet vjoule_service.service");
	if (sys != 0) {
	    throw vjoule::vjoule_error ("vJoule service is not started. sudo systemctl start vjoule_service.service");
	}

	this-> _inotifFd = inotify_init ();
	this-> _inotifFdW = inotify_add_watch (this-> _inotifFd, utils::join_path (VJOULE_DIR, "results").c_str (), IN_MODIFY | IN_CREATE);	
	
	cgroup::Cgroup c ("vjoule_api.slice/this_" + std::to_string (getpid ()));
	try {
	    c.create ();
	} catch (...) {
	    throw vjoule::vjoule_error ("failed to create cgroup, check program capabilities ? \n $ 'sudo setcap \"cap_dac_override,cap_sys_rawio+eip\"' ./usage");
	}

	if (!c.attach (getpid ())) {
	    throw vjoule::vjoule_error ("failed to create and attach process to control group.");
	}

	this-> _groups.emplace ("this", process_group (*this, "this_" + std::to_string (getpid ())));
	this-> _groups.emplace ("this_" + std::to_string (getpid ()), process_group (*this, "this_" + std::to_string (getpid ())));
	
	std::ofstream f (utils::join_path (VJOULE_DIR, "cgroups"), std::ios::app);
	f << "vjoule_api.slice/*" << std::endl;
	f.close ();

	this-> forceSig ();
	
    }

    process_group vjoule_api::create_group (const std::string & name, const std::vector <uint64_t> & pidList) {
	cgroup::Cgroup c ("vjoule_api.slice/" + name);
	c.create ();

	for (auto & p : pidList) {
	    if (!c.attach (p)) {
		throw vjoule::vjoule_error ("failed to create and attach process to control group.");
	    }
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
	this-> forceSig ();
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

    void vjoule_api::forceSig () const {
	for (uint64_t i = 0 ; i < 2 ; i++) {
	    FILE * f = fopen (utils::join_path (VJOULE_DIR, "signal").c_str (), "w");
	    fprintf (f, "%d\n", 1);
	    fflush (f);
	    fclose (f);
	    
	    usleep (10000);
	    this-> waitIteration ();
	}
    }

    void vjoule_api::waitIteration () const {
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
    
    vjoule_api::~vjoule_api () {
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
	this-> _context.forceSig ();
	
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
	cgroup::Cgroup c ("vjoule_api.slice/" + this-> _name);
	c.detachAll ();
	c.remove ();
    }
    
}


std::ostream & operator << (std::ostream & o, const vjoule::consumption_stamp_t & c) {
    o << "stamp (" << c.timestamp.time_since_epoch ().count () << ", cpu: " << c.cpu << "J, ram: " << c.ram << "J, gpu: " << c.gpu << "J)";

    return o;
}

std::ostream & operator << (std::ostream & o, const vjoule::consumption_diff_t & c) {
    o << "diff (time: " << c.duration << "s, cpu: " << c.cpu << "J, ram: " << c.ram << "J, gpu: " << c.gpu << "J)";

    return o;
}

std::ostream & operator << (std::ostream & o, const vjoule::consumption_perc_t & c) {
    o << "perc (time: " << c.duration << "%, cpu: " << c.cpu << "%, ram: " << c.ram << "%, gpu: " << c.gpu << "%)";

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
