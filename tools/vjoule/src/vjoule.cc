#include <filesystem>
#include <sys/wait.h>
#include <fstream>

#include <sensor/_.hh>
#include <common/_.hh>
#include <watcher.hh>
#include <vjoule.hh>
#include <exporter.hh>

using namespace common;

namespace tools::vjoule {
    
    VJoule::VJoule(const CommandLine & cmd) :
	_cmd (cmd), _cgroup ("vjoule_xp.slice/process_" + std::to_string (getpid ()))
    {
	std::string current_path = std::filesystem::current_path().string(); 

	this-> _vjoule_directory = utils::join_path(current_path, "__vjoule");
	std::filesystem::create_directories (this-> _vjoule_directory);

	this-> _working_directory = utils::join_path(this-> _vjoule_directory, "latest");
	this-> _cfg_path = utils::join_path(this-> _working_directory, "config.toml");

	std::vector<std::string> subargs;
	if (this-> _cmd.subCmd.size () >= 2) {
	    subargs.insert (subargs.end (), this-> _cmd.subCmd.begin () + 1, this-> _cmd.subCmd.end ());
	}
	
	this->_child = concurrency::SubProcess(this-> _cmd.subCmd [0], subargs, ".");

	this-> _cgroup.create ();
	
	this-> create_result_directory();
	this-> create_configuration();
    }

    void VJoule::create_default_config() {
	std::ofstream ofs (this-> _cfg_path, std::ofstream::out);
	ofs << "[sensor]" << std::endl;
	ofs << "freq = 1 # frequency of update in hertz (the higher the faster)" << std::endl;
	ofs << "log-lvl = \"info\" # debug < success < info < warn < error < none" << std::endl;
	ofs << "log-path = \"" << utils::join_path(this-> _vjoule_directory, "logs") << "\" # log file (empty means stdout)" << std::endl;
	ofs << "core = \"divider\" # the name of the core plugin to use for the sensor" << std::endl;
	ofs << "output-dir = \"" << this-> _working_directory << "\"" <<std::endl;
	ofs << "cgroups = \"" <<  utils::join_path(this-> _working_directory, "cgroups") << "\"" << std::endl;
	ofs << "mount-tmpfs = false" << std::endl;
	ofs << std::endl;
	if (this-> _cmd.cpu && this-> _cmd.rapl) {
	    ofs << "[cpu] # configuration to enable CPU energy reading" << std::endl;
	    ofs << "name = \"rapl\" # rapl plugin for compatible intel or amd cpus" << std::endl;
	    ofs << std::endl;
	}
	if (this-> _cmd.ram && this-> _cmd.rapl) {
	    ofs << "[ram] # configuration to enable RAM energy reading" << std::endl;
	    ofs << "name = \"rapl\" # rapl plugin for compatible intel or amd cpus" << std::endl;
	    ofs << std::endl;
	}
	if (this-> _cmd.gpu && this-> _cmd.nvidia) {
	    ofs << "[gpu:0] # configuration to enable GPU energy reading" << std::endl;
	    ofs << "name = \"nvidia\" # nvidia plugin for nvidia GPUs" << std::endl;
	    ofs << std::endl;
	}
	
	if (this-> _cmd.gpu && this-> _cmd.rapl) {
	    ofs << "[gpu:1] # configuration to enable GPU energy " << std::endl;
	    ofs << "name = \"rapl\" # rapl plugin form compatible intel of amd cpus" << std::endl;
	}
    }

    void VJoule::create_default_cgroups_list() {
	std::ofstream ofs (utils::join_path(this-> _working_directory, "cgroups"), std::ofstream::out);
	ofs << this-> _cgroup.getName () << std::endl;
    }

    void VJoule::create_configuration() {
	this-> create_default_config();	
	this-> create_default_cgroups_list();
    }

    void VJoule::create_result_directory() {
	std::string result_dir = utils::join_path(this-> _vjoule_directory, utils::get_time_no_space());
	std::filesystem::create_directories(result_dir);
    
	if (utils::file_exists(this-> _working_directory)) {
	    std::filesystem::remove(this-> _working_directory);
	}
	std::filesystem::create_directory_symlink(result_dir, this-> _working_directory);
    }

    void VJoule::run() {
	char vjoule_exe_name[] =  "vjoule_service";
	char vjoule_c_flag[] = "-c", *ccontent = const_cast<char*>(this-> _cfg_path.c_str());
	char vjoule_l_flag[] = "-l", lcontent[] = "";
	char vjoule_v_flag[] = "-v", *vcontent = const_cast<char*> (this-> _cmd.verbose ? "info" : "warn");
	std::vector<char *> args = {
	    vjoule_exe_name,
	    vjoule_c_flag, ccontent,
	    vjoule_l_flag, lcontent,
	    vjoule_v_flag, vcontent
	};
		
	::sensor::Sensor s (args.size(), args.data());

	do {
	    s.forcedIteration();
	} while (!utils::file_exists(utils::join_path(this-> _working_directory, this-> _cgroup.getName () + "/cpu")));
	
	Exporter e (this-> _working_directory, this-> _cgroup.getName (), this-> _cmd);
	pid_t c_pid = fork();
	
	if (c_pid == -1) {
	    printf("Could not fork to execute vjoule_exec");
	    exit(EXIT_FAILURE);
	} else if (c_pid > 0) {
	    // parent process
	    s.runAsync();

	    // wait for child process to finish
	    int status;
	    wait (&status);

	    s.forcedIteration();

	    if (strcmp (this-> _cmd.output.c_str(), "") == 0) {
		e.export_stdout();
	    } else {
		e.export_csv (this-> _cmd.output);
	    }

	    // Clean the cgroup created by the vjoule command line
	    this-> _cgroup.remove ();
	    
	    // if the parent contains no running xp, we remove it to avoid system pollution
	    cgroup::Cgroup parent ("vjoule_xp.slice");
	    if (!parent.isSlice ()) { 
		parent.remove ();
	    }	    
	} else {
	    // child process
	    // attach itself to cgroup
	    if (!this-> _cgroup.attach (getpid ())) {
		std::cerr << "cgroup change of group failed." << std::endl;
		exit (-1);
	    }
	    
	    this-> _child.start (false);	    
	    auto code = this-> _child.wait();

	    if (code != 0) {
		std::cerr << "Command exited with non zero status " << code << std::endl;
	    }
	    
	    std::cout << std::endl << std::endl;	   	    
	}
    }
}
