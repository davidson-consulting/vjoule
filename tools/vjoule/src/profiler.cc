#include <sensor/_.hh>
#include "profiler.hh"
#include <filesystem>
#include <sys/inotify.h>
#include <sys/sysinfo.h>

using namespace common;

#define PBSTR "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"
#define PBWIDTH 60


void printProgress(double percentage) {
    int val = (int) (percentage * 100);
    int lpad = (int) (percentage * PBWIDTH);
    int rpad = PBWIDTH - lpad;
    printf("\r%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
    fflush(stdout);
}

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

namespace tools::vjoule {

    Profiler::Profiler (const CommandLine & cmd)
	: _cmd (cmd), _nbIter (5)
    {
	std::string cwd = std::filesystem::current_path().string(); 

	this-> _vjouleDir =  utils::join_path ("/tmp/vjoule_prof/", utils::get_time_no_space());
	std::filesystem::create_directories (this-> _vjouleDir);
	
	this-> _cfgPath = common::utils::join_path(this-> _vjouleDir, "config.toml");

	this-> createConfiguration ();
    }

    void Profiler::createConfiguration () const {
	std::ofstream ofs (this-> _cfgPath, std::ofstream::out);
	ofs << "[sensor]" << std::endl;
	ofs << "freq = 1 # frequency of update in hertz (the higher the faster)" << std::endl;
	ofs << "log-lvl = \"info\" # debug < success < info < warn < error < none" << std::endl;
	ofs << "log-path = \"" << common::utils::join_path(this-> _vjouleDir, "logs") << "\" # log file (empty means stdout)" << std::endl;
	ofs << "core = \"divider\" # the name of the core plugin to use for the sensor" << std::endl;
	ofs << "output-dir = \"" << common::utils::join_path(this-> _vjouleDir, "results") << "\"" <<std::endl;
	ofs << "cgroups = \"" <<  common::utils::join_path(this-> _vjouleDir, "cgroups") << "\"" << std::endl;
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
	ofs.close ();
	
	std::ofstream ofs2 (utils::join_path (this-> _vjouleDir, "cgroups"));
	ofs2 << std::endl;
	ofs2.close ();
    }

    void Profiler::readConsumption (float & cpu, float & ram) const {
	auto ramPath = utils::join_path (this-> _vjouleDir, "results/ram");
	auto cpuPath = utils::join_path (this-> _vjouleDir, "results/cpu");

	if (std::filesystem::exists (ramPath)) {
	    std::ifstream ramF (ramPath);	
	    ramF >> ram;
	    ramF.close ();
	}

	if (std::filesystem::exists (cpuPath)) {
	    std::ifstream cpuF (cpuPath);	
	    cpuF >> cpu;
	    cpuF.close ();
	}
    }

    void Profiler::run () {
	char * cfgPath = const_cast<char*>(this-> _cfgPath.c_str());
	char vjoule_exe_name[] =  "vjoule_service";
	char vjoule_l_flag[] = "-l", lcontent[] = "";
	char vjoule_c_flag[] = "-c", *ccontent = cfgPath;
	char vjoule_v_flag[] = "-v", *vcontent = const_cast<char*> (this-> _cmd.verbose ? "info" : "warn");
	char * args[] = {
	    vjoule_exe_name,
	    vjoule_c_flag, ccontent,
	    vjoule_l_flag, lcontent,
	    vjoule_v_flag, vcontent
	};
	
	::sensor::Sensor s;
	s.configure (7, args);
	
	std::vector <ResultRow> results;
	concurrency::timer t;
	for (uint64_t i = 0 ; i < get_nprocs () ; i++) {
	    printProgress ((double) i / (double) get_nprocs ());
	    s.forcedIteration ();	    
	    float ocpu = 0, oram = 0;
	    this-> readConsumption (ocpu, oram);

	    t.reset ();
	    this-> runLoad (i + 1);
	    auto time = t.time_since_start ();
	    
	    s.forcedIteration ();
	    float cpu = 0, ram = 0;
	    this-> readConsumption (cpu, ram);

	    t.sleep (1);
	    t.reset ();
	    results.push_back ({
		    (cpu - ocpu) / time,
		    (ram - oram) / time
		});
	}

	printProgress (1);
	this-> printResult (results);
    }
    
    void Profiler::printResult (const std::vector <ResultRow> & res) const {
	printf ("\n");
	printf ("%10s | %8s  | %8s  |\n", "Nb cores", "CPU", "RAM");
	printf (" %9c | %8c  | %8c  |\n", '-', '-', '-');
	for (uint64_t i = 0 ; i < res.size () ; i++) {
	    printf ("%10ld | %8.2fW | %8.2fW |\n", i, res[i].cpu, res[i].ram);
	}	
    }

    void Profiler::runLoad (uint64_t nb) {
	std::vector <std::string> args = {"-c", std::to_string (nb), "--timeout", "1s"};
	auto proc = concurrency::SubProcess ("stress", args, ".");
	proc.start ();
	proc.wait ();
    }
    
}
