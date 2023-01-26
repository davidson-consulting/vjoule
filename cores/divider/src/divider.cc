#include "divider.hh"

#include <filesystem>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>


#include <mntent.h>

using namespace common;

namespace divider {

    Divider::Divider () {}

    bool Divider::configure (const common::utils::config::dict & cfg, common::plugin::Factory & factory) {
	this-> _cgroupFile = utils::join_path (VJOULE_DIR, "cgroups");
	this-> _outputDir = cfg.getOr <std::string> ("output-dir", "/etc/vjoule/results");
	this-> _deleteRes = cfg.getOr <bool> ("delete-res", true);

	this-> mountResultDir ();

	bool v2 = false;
	this-> _cgroupRoot = utils::get_cgroup_mount_point (v2);
	if (!v2) {
	    LOG_ERROR ("Cgroup v2 not mounted, only cgroup v2 is supported.");
	    return false;
	}

	LOG_INFO ("Cgroup v2 detected @ ", this-> _cgroupRoot);
	
	if (!this-> configureGpuPlugins (factory)) return false;
	if (!this-> configureCpuPlugin (factory)) return false;
	if (!this-> configureRamPlugin (factory)) return false;

	auto nbMaxOpen = cfg.getOr <int> ("max-open-files", 65535);
	struct rlimit r;
	r.rlim_cur = nbMaxOpen;
	r.rlim_max = nbMaxOpen;

	LOG_INFO ("Setting max open files : ", nbMaxOpen);
	setrlimit (RLIMIT_NOFILE, &r);
	getrlimit (RLIMIT_NOFILE, &r);

	if (r.rlim_cur != nbMaxOpen) {
	    LOG_ERROR ("Failed setting max open files, current max is : ", r.rlim_cur);
	    return false;
	}	

	this-> _notif.onUpdate ().connect (this, &Divider::onCgroupUpdate);
	this-> configureCgroups ();
		
	return true;
    }

    void Divider::onCgroupUpdate () {
	this-> _needUpdate = true;
    }
    
    bool Divider::configureGpuPlugins (common::plugin::Factory & factory) {
	this-> _gpuPlugins = factory.getPlugins ("gpu");
	for (auto & it : this-> _gpuPlugins) {
	    auto poll = it-> getFunction<common::plugin::PluginPollFunc_t> ("poll");
	    if (poll == nullptr) {
		LOG_ERROR ("Invalid 'gpu' plugin '", it-> getName (), "' has no 'void poll ()' function");
		return false;
	    }
	    
	    this-> _pollFunctions.emplace (poll);
	    auto nbDevice = it-> getFunction <common::plugin::GpuGetNbDevices_t> ("gpu_nb_devices");
	    if (nbDevice == nullptr) {
		LOG_ERROR ("Invalid 'gpu' plugin '", it-> getName (), "' has no 'uint32_t gpu_nb_devices ()' function");
		return false;
	    }

	    std::vector <float> cache;
	    cache.resize (nbDevice ());
	    this-> _gpuEnergyCache.push_back (std::move (cache));


	    auto get = it-> getFunction <common::plugin::GpuGetEnergy_t> ("gpu_get_energy");
	    if (get == nullptr) {
		LOG_ERROR ("Invalid 'gpu' plugin '", it-> getName (), "' has no 'void gpu_get_energy (float * energyDevices)' function");
		return false;
	    }
	    this-> _gpuGet.push_back (get);

	    auto perf_event = it-> getFunction<common::plugin::GpuGetDeviceUsage_t> ("gpu_cgroup_usage");
	    if (perf_event == nullptr) {
		LOG_ERROR ("Invalid 'gpu' plugin '", it-> getName (), "' has no 'float gpu_cgroup_usage (uint32_t device, const char* cgroupName)' function");
		return false;
	    }
	    this-> _gpuPerfEvents.push_back (perf_event);	    
	}

	if (this-> _gpuPlugins.size () == 0) {
	    LOG_INFO ("No 'gpu' plugin in use");
	}

	
	return true;
    }

    bool Divider::configureCpuPlugin (common::plugin::Factory & factory) {
	auto cpus = factory.getPlugins ("cpu");
	if (cpus.size () > 0) {
	    this-> _cpuPlugin = cpus[0];
	    if (cpus.size () > 1) LOG_WARN ("Core 'divider' can manage only one CPU plugin.");
	    auto poll = this-> _cpuPlugin-> getFunction<common::plugin::PluginPollFunc_t> ("poll");
	    if (poll == nullptr) {
		LOG_ERROR ("Invalid 'cpu' plugin '", this-> _cpuPlugin-> getName (), "' has no 'void poll ()' function");
		return false;
	    }
	    this-> _pollFunctions.emplace (poll);

	    auto get = this-> _cpuPlugin-> getFunction <common::plugin::CpuGetEnergy_t> ("cpu_get_energy");
	    if (get == nullptr) {
		LOG_ERROR ("Invalid 'cpu' plugin '", this-> _cpuPlugin-> getName (), "' has no 'float cpu_get_energy ()' function");
		return false;
	    }
	    this-> _cpuGet = get;	    
	} else {
	    this-> _cpuPlugin = nullptr;
	    LOG_INFO ("No 'cpu' plugin in use");
	}
	
	return true;
    }

    bool Divider::configureRamPlugin (common::plugin::Factory & factory) {
	auto rams = factory.getPlugins ("ram");
	if (rams.size () > 0) {
	    this-> _ramPlugin = rams[0];
	    if (rams.size () > 1) LOG_WARN ("Core 'divider' can manage only one RAM plugin.");
	    auto poll = this-> _ramPlugin-> getFunction<common::plugin::PluginPollFunc_t> ("poll");
	    if (poll == nullptr) {
		LOG_ERROR ("Invalid 'ram' plugin '", this-> _ramPlugin-> getName (), "' has no 'void poll ()' function");
		return false;
	    }
	    
	    this-> _pollFunctions.emplace (poll);

	    auto get = this-> _ramPlugin-> getFunction <common::plugin::CpuGetEnergy_t> ("ram_get_energy");
	    if (get == nullptr) {
		LOG_ERROR ("Invalid 'cpu' plugin '", this-> _ramPlugin-> getName (), "' has no 'float ram_get_energy ()' function");
		return false;
	    }
	    this-> _ramGet = get;
	} else {
	    this-> _ramPlugin = nullptr;
	    LOG_INFO ("No 'ram' plugin in use");
	}

	return true;
    }
    
    void Divider::compute () {
	if (this-> _needUpdate) {
	    this-> configureCgroups ();
	    this-> _needUpdate = false;
	}
	
	for (auto & it : this-> _pollFunctions) {
	    it ();
	}
	
	if (this-> _cpuPlugin != nullptr || this-> _ramPlugin != nullptr) {
	    this-> pollPerfEvents ();
	}
	
	this-> computeCpuEnergy ();	
	this-> computeRamEnergy ();    
	this-> computeGpuEnergy ();

	this-> writeConsumption ();
    }

    
    void Divider::writeConsumption () {
	for (auto & it : this-> _results) {
	    fseek (it.second.cpuFd, 0, SEEK_SET);
	    fprintf (it.second.cpuFd, "%lf", it.second.cpu);
	    fflush (it.second.cpuFd);

	    fseek (it.second.ramFd, 0, SEEK_SET);
	    fprintf (it.second.ramFd, "%lf", it.second.ram);
	    fflush (it.second.ramFd);

	    fseek (it.second.gpuFd, 0, SEEK_SET);
	    fprintf (it.second.gpuFd, "%lf", it.second.gpu);
	    fflush (it.second.gpuFd);
	}
    }

    void Divider::pollPerfEvents () {
	for (uint64_t i = 0 ; i < this-> _cgroupList.size () ; i++) {
	    this-> _cgroupWatchers[i].poll (this-> _perfEventValues[i]);
	}       
    }

    void Divider::computeGpuEnergy () {
	for (uint64_t i = 0 ; i < this-> _gpuGet.size () ; i++) {
	    this-> _gpuGet[i](this-> _gpuEnergyCache[i].data ());
	    for (uint64_t j = 0 ; j < this-> _cgroupList.size () ; j++) {		  
		for (uint64_t device = 0 ; device < this-> _gpuEnergyCache[i].size () ; device++) {
		    auto use = this-> _gpuPerfEvents[i](device);
		    if (j == 0) {
			auto & cgroup = this-> _results[this-> _cgroupList[0].getName ()];
			cgroup.gpu += this-> _gpuEnergyCache[i][device];
		    } else {
			auto & name = this-> _cgroupList[j].getName ();
			auto jt = use.find (name);
			if (jt != use.end ()) {
			    auto & cgroup = this-> _results[name];
			    cgroup.gpu += (this-> _gpuEnergyCache[i][device] * jt-> second);
			}
		    }
		}
	    }
	}	
    }

    void Divider::computeRamEnergy () {
	if (this-> _ramGet != nullptr) {
	    float ramEnergy = this-> _ramGet ();
	    
	    // LLC_MISSES of the system
	    auto ident = (float) (this-> _perfEventValues[0][1]);
	    this-> _results[""].ram += ramEnergy;
	    for (uint64_t i = 1 ; i < this-> _perfEventValues.size () ; i++) {
		auto & cgroup = this-> _results[this-> _cgroupList[i].getName ()];
		cgroup.ram += ramEnergy * ((float) (this-> _perfEventValues[i][1]) / ident);
	    }
	}
    }

    void Divider::computeCpuEnergy () {
	if (this-> _cpuGet != nullptr) {
	    float cpuEnergy = this-> _cpuGet ();
	    
	    // LLC_MISSES of the system
	    auto ident = (float) (this-> _perfEventValues[0][0]);
	    this-> _results[""].cpu += cpuEnergy;
	    for (uint64_t i = 1 ; i < this-> _perfEventValues.size () ; i++) {
		auto & cgroup = this-> _results[this-> _cgroupList[i].getName ()];
		cgroup.cpu += cpuEnergy * ((float) (this-> _perfEventValues[i][0]) / ident);
	    }
	}
    }
   
    void Divider::configureCgroups () {
	std::vector <std::string> events = {"PERF_COUNT_HW_CPU_CYCLES", "LLC_MISSES"};

	std::vector <common::cgroup::Cgroup> rest;
	std::vector <perf::PerfEventWatcher> watchers;
	std::vector <std::vector <uint64_t> > cache;
	bool empty = false;

	auto rules = this-> readCgroupRules ();
	cgroup::CgroupLister lister (rules);
	auto lst = lister.run ();
	
	for (uint64_t i = 0 ; i < this-> _cgroupList.size () ; i++) {
	    auto jt = lst.find (this-> _cgroupList[i]);
	    if (this-> _cgroupList[i].getName () == "") empty = true;
	    if (jt != lst.end ()) {
		rest.push_back (this-> _cgroupList [i]);
		watchers.push_back (std::move (this-> _cgroupWatchers[i]));
		cache.push_back (std::move (this-> _perfEventValues [i]));
		lst.erase (this-> _cgroupList[i]);
	    } else {
		this-> _cgroupWatchers[i].dispose ();
		if (this-> _deleteRes) {
		    this-> removeResultDirectory (this-> _cgroupList[i]);
		}
	    }
	}
		
	this-> _cgroupList = std::move (rest);
	this-> _cgroupWatchers = std::move (watchers);
	this-> _perfEventValues = std::move (cache);
	
	if (!empty) {
	    lst.emplace (common::cgroup::Cgroup (""));
	}
	
	for (auto & it : lst) {
	    perf::PerfEventWatcher pw (it.getName ());
	    pw.configure (events);
	    this-> _cgroupWatchers.push_back (std::move (pw));
	    this-> _perfEventValues.push_back({});
	    this-> _perfEventValues.back ().resize (events.size ());
	    this-> _cgroupList.push_back (it);

	    this-> createResultDirectory (it);
	}

	this-> _notif.configure (this-> _cgroupFile, rules);
	this-> _notif.start ();
    }

    
    std::vector <std::string> Divider::readCgroupRules () const {       
	std::ifstream infile(this-> _cgroupFile);
	std::string line;
	std::vector <std::string> rules;
	while (std::getline(infile, line)) {
	    bool use = false;
	    for (auto & it : line) {
		if (!std::isblank (it)) {
		    use = true;
		    break;
		}
	    }

	    if (use) {
		bool at_least_slice = line.find ('/') != std::string::npos;	    
		if (!at_least_slice) {
		    LOG_WARN ("Cgroup rule '", line, "' ignored, watched cgroup must be placed inside a slice, maybe you meant : ", line, "/*");
		} else {
		    rules.push_back (line);
		}
	    }
	}

	return rules;
    }

    void Divider::mountResultDir () {
	// std::cout << utils::get_mount_type (this-> _outputDir) << std::endl;
    }        

    void Divider::createResultDirectory (const common::cgroup::Cgroup & c) {
	LOG_INFO ("Create result directory for cgroup [", c.getName (), "]");
	CgroupResult r {};
	std::string resultDir = "";
	if (c.getName ().size () > this-> _cgroupRoot.length ()) {
	    resultDir = c.getName ().substr (this-> _cgroupRoot.length ());
	}
	
	resultDir = utils::join_path (this-> _outputDir, resultDir);
	std::filesystem::create_directories (resultDir);
	
	r.cpuFd = fopen (utils::join_path (resultDir, "cpu").c_str (), "w");
	r.ramFd = fopen (utils::join_path (resultDir, "ram").c_str (), "w");
	r.gpuFd = fopen (utils::join_path (resultDir, "gpu").c_str (), "w");

	this-> _results[c.getName ()] = r;
    }

    void Divider::removeResultDirectory (const common::cgroup::Cgroup & c) {
	LOG_INFO ("Delete result directory for cgroup [", c.getName (), "]");
	auto r = this-> _results[c.getName()];
	if (r.cpuFd != nullptr) fclose (r.cpuFd);
	if (r.ramFd != nullptr) fclose (r.ramFd);
	if (r.gpuFd != nullptr) fclose (r.gpuFd);

	std::string resultDir = "";
	if (c.getName ().size () > this-> _cgroupRoot.length ()) {
	    resultDir = c.getName ().substr (this-> _cgroupRoot.length ());
	}	
	resultDir = utils::join_path (this-> _outputDir, resultDir);

	this-> _results.erase (c.getName ());
	std::filesystem::remove (utils::join_path (resultDir, "cpu").c_str ());
	std::filesystem::remove (utils::join_path (resultDir, "ram").c_str ());
	std::filesystem::remove (utils::join_path (resultDir, "gpu").c_str ());

	while (std::filesystem::is_empty (resultDir)) {
	    std::filesystem::remove (resultDir);
	    resultDir = utils::parent_directory (resultDir);
	}
    }


    void Divider::dispose () {
	this-> _cgroupWatchers.clear ();
    }
    

    
}
