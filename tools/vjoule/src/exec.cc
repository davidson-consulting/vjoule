#include <sys/wait.h>
#include <fstream>

#include <watcher.hh>
#include <exec.hh>
#include <exiting.hh>

#include <unistd.h>
#include <fcntl.h>
#include <string.h>


using namespace common;

namespace tools::vjoule {
    
    Exec::Exec(const CommandLine & cmd) :
		_cmd (cmd), _cgroup ("vjoule_xp.slice/process_" + std::to_string (getpid ()))
    {
		std::string current_path = utils::current_directory ();

		this-> _vjoule_directory = utils::join_path(current_path, "__vjoule");
		utils::create_directory (this-> _vjoule_directory, true);

		this-> _working_directory = utils::join_path(this-> _vjoule_directory, "latest");
		this-> _cfg_path = utils::join_path(this-> _working_directory, "config.toml");
		this-> _timeStart = utils::get_time_no_space();

		std::vector<std::string> subargs;
		if (this-> _cmd.subCmd.size () >= 2) {
			subargs.insert (subargs.end (), this-> _cmd.subCmd.begin () + 1, this-> _cmd.subCmd.end ());
		}
	
		this->_child = concurrency::SubProcess(this-> _cmd.subCmd [0], subargs, ".");
	
		this-> create_result_directory();
		this-> create_configuration();
    }

    void Exec::create_default_config() {
		std::ofstream ofs (this-> _cfg_path, std::ofstream::out);
		ofs << "[sensor]" << std::endl;
		ofs << "freq = 1 # frequency of update in hertz (the higher the faster)" << std::endl;
		ofs << "log-lvl = \"info\" # debug < success < info < warn < error < none" << std::endl;
		ofs << "log-path = \"" << utils::join_path(this-> _vjoule_directory, "logs") << "\" # log file (empty means stdout)" << std::endl;
		ofs << "core = \"divider\" # the name of the core plugin to use for the sensor" << std::endl;
		ofs << "output-dir = \"" << this-> _working_directory << "\"" <<std::endl;
		ofs << "cgroups = \"" <<  utils::join_path(this-> _working_directory, "cgroups") << "\"" << std::endl;
		ofs << "signal-path = \"" <<  utils::join_path(this-> _working_directory, "signal") << "\"" << std::endl;
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

    void Exec::create_default_cgroups_list() {
		std::ofstream ofs (utils::join_path(this-> _working_directory, "cgroups"), std::ofstream::out);
		ofs << this-> _cgroup.getName () << std::endl;
    }

    void Exec::create_configuration() {
		this-> create_default_config();
		this-> create_default_cgroups_list();
    }

    void Exec::create_result_directory() {
		std::string result_dir = utils::join_path(this-> _vjoule_directory, this-> _timeStart);
		utils::create_directory(result_dir, true);
    
		if (utils::file_exists(this-> _working_directory)) {
			utils::remove (this-> _working_directory);
		}

		utils::create_symlink (this-> _working_directory, result_dir);
    }

    void Exec::dispose () {
		try {
			if (this-> _started) {
				if (!this-> _exported) {
					this-> exportResults ();
				}
		
				this-> _sensor.stop ();
			}
	    
			if (this-> _childPid != 0) {
				kill (9, this-> _childPid);
		
				int status;
				waitpid (this-> _childPid, &status, 0);
			}
	    
			// Detach all the pids attached to the cgroup
			this-> _cgroup.detachAll ();
	    
			// Clean the cgroup created by the vjoule command line
			this-> _cgroup.remove ();
	    
			// if the parent contains no running xp, we remove it to avoid system pollution
			cgroup::Cgroup parent ("vjoule_xp.slice");
			if (!parent.isSlice ()) {
				parent.remove ();
			}
		} catch (...) {
			std::cerr << "Error when cleaning cgroups" << std::endl;
		}
    }

    void Exec::exportResults () {
		this-> _sensor.forcedIteration();

		if (strcmp (this-> _cmd.output.c_str(), "") == 0) {
			this-> _exporter.export_stdout();
		} else {
			this-> _exporter.export_csv (this-> _cmd.output);
		}

		this-> _exported = true;
    }
    
    void Exec::run() {		
		int pipes[2];
		if (pipe (pipes) == -1) {
			std::cout << "Error .." << std::endl;
		};

		pid_t c_pid = fork();
		if (c_pid == -1) {
			printf("Could not fork to execute vjoule_exec");
			throw ExecError ();
		} else if (c_pid > 0) {
			close (pipes[0]);
			this-> runParent (c_pid, pipes[1]);
		} else {
			close (pipes[1]);
			this-> runChild (pipes[0]);
		}
    }
    
    void Exec::runParent (uint64_t childPid, int pipe) {
		try {
			this-> _childPid = childPid;
			this-> _cgroup.create ();

			exitSignal.connect (this, &Exec::dispose);
	    
			if (!this-> _cgroup.attach (childPid)) {
				std::cerr << "error: failed to attach pid of command to monitored cgroup." << std::endl;
				throw ExecError ();
			}

			for (auto & pid : this-> _cmd.pids) {
				if (!this-> _cgroup.attach (pid)) {
					std::cerr << "warning: failed to attach pid '" << pid << "' to monitored cgroup." << std::endl;
				}
			}

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
		
			this-> _sensor.configure (args.size(), args.data());
			this-> _started = true;
	    
			do {
				this-> _sensor.forcedIteration();
			} while (!utils::file_exists(utils::join_path(this-> _working_directory, this-> _cgroup.getName () + "/cpu")));
	    
			this-> _exporter.configure (this-> _working_directory, this-> _cgroup.getName (), this-> _cmd, this-> _timeStart);
			this-> _sensor.runAsync();

			if (write (pipe, "GO!", strlen ("GO!")) == -1) {
				std::cerr << "internal error: failed to warn child process to start." << std::endl;
				kill (childPid, 9);
			}
			close (pipe);
	
			// wait for child process to finish
			int status;
			waitpid (childPid, &status, 0);
			this-> exportResults ();
	    
			this-> _sensor.stop ();
			this-> _started = false;
		} catch (...) {
			std::cerr << "error: failed to configure vjoule execution." << std::endl;
			this-> _cgroup.detachAll ();
			kill (childPid, 9);
	    
			int status;
			waitpid (childPid, &status, 0);
		}

		this-> dispose ();
    }

    void Exec::runChild (int pipe) {
		int c = 0;
		char buf;
		while (c = read (pipe, &buf, 1) > 0) {}
		close (pipe);
	
		// child process
		// attach itself to cgroup

		std::stringstream ss;
		for (auto & it : this-> _cmd.subCmd) {
			ss << it << " ";
		}
	
		// this-> _child.start (false);
		// auto code = this-> _child.wait();

		auto code = system (ss.str ().c_str ());
	
		if (code != 0) {
			std::cerr << "Command exited with non zero status " << code << std::endl;
		}
	    
		std::cout << std::endl << std::endl;
    }
}
