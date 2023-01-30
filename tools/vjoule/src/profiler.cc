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
	: _cmd (cmd), _nbIter (10)
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
	if (this-> _cmd.gpu && this-> _cmd.nvidia) {
	    ofs << "[gpu:0] # configuration to enable GPU energy reading" << std::endl;
	    ofs << "name = \"nvidia\" # nvidia plugin for nvidia GPUs" << std::endl;
	    ofs << std::endl;
	}
	
	if (this-> _cmd.gpu && this-> _cmd.rapl) {
	    ofs << "[gpu:1] # configuration to enable GPU energy " << std::endl;
	    ofs << "name = \"rapl\" # rapl plugin form compatible intel of amd cpus" << std::endl;
	}
	ofs.close ();
	
	std::ofstream ofs2 (utils::join_path (this-> _vjouleDir, "cgroups"));
	ofs2 << std::endl;
	ofs2.close ();
    }

    void Profiler::waitServiceIteration () const {
	auto resultPath = utils::join_path (this-> _vjouleDir, "results/cpu");
	if (!std::filesystem::exists (resultPath)) {
	    throw std::runtime_error ("Divider service is not running");
	}
    
	auto fd = inotify_init (); 
	auto wd = inotify_add_watch (fd, utils::join_path (this-> _vjouleDir, "results").c_str (), IN_MODIFY); 
    
	char buffer[EVENT_BUF_LEN];
	while (true) {	
	    auto len = read (fd, buffer, EVENT_BUF_LEN); 
	    if (len == 0) { 
		std::cerr << "waiting inotify" << std::endl;
		exit (-1);
	    }

	    int i = 0;
	    while (i < len) { 
		struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];

		if (event-> len != 0 && event-> mask & IN_MODIFY && strcmp (event-> name, "cpu") == 0) {
		    inotify_rm_watch (fd, wd); 
		    close (fd); 
		    return; 
		}
	    
		i += EVENT_SIZE + event->len;
	    }	
	}    
    }

    void Profiler::readConsumption (float & cpu, float & gpu, float & ram) const {
	auto ramPath = utils::join_path (this-> _vjouleDir, "results/ram");
	auto cpuPath = utils::join_path (this-> _vjouleDir, "results/cpu");
	auto gpuPath = utils::join_path (this-> _vjouleDir, "results/gpu");

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

	if (std::filesystem::exists (gpuPath)) {
	    std::ifstream gpuF (cpuPath);	
	    gpuF >> gpu;
	    gpuF.close ();
	}
    }

    void Profiler::run () {
	char * cfgPath = const_cast<char*>(this-> _cfgPath.c_str());
	char vjoule_exe_name[] =  "vjoule_service";
	char vjoule_l_flag[] = "-l", lcontent[] = "";
	char vjoule_c_flag[] = "-c", *ccontent = cfgPath;
	char vjoule_v_flag[] = "-v", vcontent[] = "warn";
	char * args[] = {
	    vjoule_exe_name,
	    vjoule_c_flag, ccontent,
	    vjoule_l_flag, lcontent,
	    vjoule_v_flag, vcontent
	};
	
	::sensor::Sensor s (7, args);
	
	std::vector <ResultRow> results;
	concurrency::timer t;
	for (uint64_t i = 0 ; i < get_nprocs () ; i++) {	    
	    s.forcedIteration ();
	    t.reset ();
	    float ocpu = 0, ogpu = 0, oram = 0;
	    this-> readConsumption (ocpu, ogpu, oram);

	    this-> runLoad (i + 1);
	    printProgress ((double) i / (double) get_nprocs ());
	    s.forcedIteration ();
	    float cpu = 0, gpu = 0, ram = 0;
	    this-> readConsumption (cpu, gpu, ram);

	    auto time = t.time_since_start ();
	    t.reset ();
	    results.push_back ({
		(cpu - ocpu) / time,
		 (gpu - ogpu) / time,
		 (ram - oram) / time
		});
	}

	this-> printResult (results);
    }
    
    void Profiler::printResult (const std::vector <ResultRow> & res) const {
	printf ("\n");
	if (this-> _cmd.cpu && this-> _cmd.ram && this-> _cmd.gpu) {
	    printf ("%10s | %8s | %8s | %8s |\n", "Nb cores", "CPU", "RAM", "GPU");
	    printf (" %9c | %8c | %8c | %8c |\n", '-', '-', '-', '-');
	    for (uint64_t i = 0 ; i < res.size () ; i++) {
		printf ("%10ld | %8.5f | %8.5f | %8.5f |\n", i, res[i].cpu, res[i].ram, res[i].gpu);
	    }
	}
    }

    void Profiler::runLoad (uint64_t nb) {
	this-> _allRes = 0;
	std::vector <concurrency::thread> ths;
	for (uint64_t i = 0 ; i < nb ; i++) {
	    ths.push_back (concurrency::spawn (this, &Profiler::computePi));
	}

	for (uint64_t i = 0 ; i < nb ; i++) {
	    concurrency::join (ths[i]);
	}
    }

    void Profiler::computePi (common::concurrency::thread) {
	double res = 0.0f;
	for (uint64_t i = 0 ; i < this-> _nbIter ; i++) {
	    res = res + vjoule::computePi (10000000); 
	}

	this-> _allRes += res / (double) this-> _nbIter;
    }    

    double computePi (uint64_t prec) {
	double res = 0;
	double prec_d = (double) prec;
	for (uint64_t i = 0  ; i < prec ; i++) {
	    res += (4.0 / prec_d) / (1.0 + (((double) (i) - 0.5) * (1.0 / prec_d)) * (((double) (i) - 0.5) * (1.0 / prec_d)));
	}
	
	return res;	
    }
    
}
