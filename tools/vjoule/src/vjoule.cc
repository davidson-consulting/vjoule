#include <filesystem>
#include <libcgroup.h>
#include <sys/wait.h>
#include <fstream>

#include <sensor/_.hh>
#include <common/_.hh>
#include <watcher.hh>
#include <vjoule.hh>
#include <exporter.hh>

namespace tools::vjoule {

    VJoule::VJoule(const CommandLine & cmd): _cmd (cmd) {
	std::string current_path = std::filesystem::current_path().string(); 

	this-> _vjoule_directory = common::utils::join_path(current_path, "__vjoule");
	this-> _cfg_path = common::utils::join_path(this-> _vjoule_directory, "config.toml");

	std::vector<std::string> subargs;
	if (this-> _cmd.subCmd.size () >= 2) {
	    subargs.insert (subargs.end (), this-> _cmd.subCmd.begin () + 1, this-> _cmd.subCmd.end ());
	}
	
	this->_child = common::concurrency::SubProcess(this-> _cmd.subCmd [0], subargs, ".");

	// create cgroup for running vjoule_exec
	int ret = 0;
	ret = cgroup_init();

	if (ret) {
	    fprintf(stderr, "cgroup_init failed\n");
	    exit(1);
	}

	struct cgroup *cg = NULL;
	cg = cgroup_new_cgroup("vjoule_xp.slice/process");
    
	if (!cg) {
	    fprintf(stderr, "Failed to allocate cgroup %s\n", "vjoule_xp.slice/process");
	    exit(1);
	}

	struct cgroup_controller *cgc;
	cgc = cgroup_add_controller(cg, "cpu");
	if (!cgc) {
	    printf("FAIL: cgroup_add_controller failed\n");
	    exit(3);
	}

	ret = cgroup_create_cgroup(cg, 0);
	if (ret) {
	    printf("FAIL: cgroup_create_cgroup failed with %s\n",
		   cgroup_strerror(ret));
	    exit(3);
	}

	this-> create_configuration_if_needed();
	this-> create_result_directory();
    }

    void VJoule::create_default_config() {
	std::ofstream ofs (this-> _cfg_path, std::ofstream::out);
	ofs << "[sensor]" << std::endl;
	ofs << "freq = 1 # frequency of update in hertz (the higher the faster)" << std::endl;
	ofs << "log-lvl = \"info\" # debug < success < info < warn < error < none" << std::endl;
	ofs << "log-path = \"" << common::utils::join_path(this-> _vjoule_directory, "logs") << "\" # log file (empty means stdout)" << std::endl;
	ofs << "core = \"divider\" # the name of the core plugin to use for the sensor" << std::endl;
	ofs << "output-dir = \"" << common::utils::join_path(this-> _vjoule_directory, "latest") << "\"" <<std::endl;
	ofs << "cgroups = \"" <<  common::utils::join_path(this-> _vjoule_directory, "cgroups") << "\"" << std::endl;
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
	std::ofstream ofs (common::utils::join_path(this-> _vjoule_directory, "cgroups"), std::ofstream::out);
	ofs << "vjoule_xp.slice/*" << std::endl;
    }

    void VJoule::create_configuration_if_needed() {
	std::filesystem::create_directories (this-> _vjoule_directory);

	this-> create_default_config();	
	if (!common::utils::file_exists(common::utils::join_path(this-> _vjoule_directory, "cgroups"))) {
	    this-> create_default_cgroups_list();
	}
    }

    void VJoule::create_result_directory() {
	std::string result_dir = common::utils::join_path(this-> _vjoule_directory, common::utils::get_time_no_space());
	std::filesystem::create_directories(result_dir);
    
	std::string latest_result_dir = common::utils::join_path(this-> _vjoule_directory, "latest");
	if (common::utils::file_exists(latest_result_dir)) {
	    std::filesystem::remove(latest_result_dir);
	}
	std::filesystem::create_directory_symlink(result_dir, latest_result_dir);
    }

    void VJoule::run() {
	char vjoule_exe_name[] =  "vjoule_service";
	char vjoule_c_flag[] = "-c";
	char * cfg_path = const_cast<char*>(this-> _cfg_path.c_str());
	char * args[3] = {vjoule_exe_name, vjoule_c_flag, cfg_path};
	::sensor::Sensor s (3, args);
 
	pid_t c_pid = fork();

	if (c_pid == -1) {
	    printf("Could not fork to execute vjoule_exec");
	    exit(EXIT_FAILURE);
	} else if (c_pid > 0) {
	    // parent process
	    s.runAsync();

	    // wait for child process to finish
	    int status;
	    wait(&status);

	    // wait for measurements to finish writing
	    Watcher w(common::utils::join_path(this-> _vjoule_directory, "latest/vjoule_xp.slice/process"), "cpu");
	    w.wait();

	    // read and pretty print results
	    Exporter e(common::utils::join_path(this-> _vjoule_directory, "latest"), this->_cmd.cpu, this->_cmd.gpu, this->_cmd.ram);
	    e.export_stdout();
	} else {
	    // child process
	    // attach itself to cgroup
	    int ret = cgroup_change_cgroup_path("vjoule_xp.slice/process", getpid(), (const char * const []){"cpu"});
	    if (ret) {
		printf("cgroup change of group failed\n");
		exit(ret);
	    }
      
	    std::string latest_dir = common::utils::join_path(this-> _vjoule_directory, "latest");
	    if(!common::utils::file_exists(common::utils::join_path(latest_dir, "vjoule_xp.slice/process/cpu"))) {
		Watcher w(latest_dir, "vjoule_xp.slice");
		w.wait();
	    }

	    this-> _child.start();
	    this-> _child.wait();
	}
    }
}
