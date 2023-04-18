#include "dumper.hh"

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/mount.h>

using namespace common;

namespace dumper {

    Dumper::Dumper () {}

    bool Dumper::configure (const common::utils::config::dict & cfg, common::plugin::Factory & factory) {
		this-> _cgroupFile = cfg.getOr <std::string> ("cgroups", utils::join_path (ANON_DIR, "cgroups"));
		this-> _outputDir = cfg.getOr <std::string> ("output-dir", "/etc/anon/results");

		if (!this-> configureGpuPlugins (factory)) return false;
		if (!this-> configureCpuPlugin (factory)) return false;
		if (!this-> configureRamPlugin (factory)) return false;

		bool v2 = false;
		this-> _cgroupRoot = utils::get_cgroup_mount_point (v2);
		if (!v2) {
			LOG_ERROR ("Cgroup v2 not mounted, only cgroup v2 is supported.");
			return false;
		}

		if (cfg.getOr <bool> ("mount-tmpfs", true)) {
			this-> mountResultDir ();
		}
	
		LOG_INFO ("Cgroup v2 detected @ ", this-> _cgroupRoot);
	
		auto lstPerfEvents = cfg.getOr <common::utils::config::array> ("perf-counters", {});
		for (int i = 0; i < lstPerfEvents.size(); i++) {
			this-> _perfEvents.push_back (lstPerfEvents.get<std::string> (i));
		}
	
		this-> _notif.onUpdate ().connect (this, &Dumper::onCgroupUpdate);
		this-> configureCgroups ();
		this-> configureCpuFrequencies ();

		this-> createResultsFiles ();
	
		return true;
    }

    void Dumper::onCgroupUpdate () {
		this-> _needUpdate = true;
    }
    
    bool Dumper::configureGpuPlugins (common::plugin::Factory & factory) {
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

    bool Dumper::configureCpuPlugin (common::plugin::Factory & factory) {
		auto cpus = factory.getPlugins ("cpu");
		if (cpus.size () > 0) {
			this-> _cpuPlugin = cpus[0];
			if (cpus.size () > 1) LOG_WARN ("Core 'dumper' can manage only one CPU plugin.");
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

    bool Dumper::configureRamPlugin (common::plugin::Factory & factory) {
		auto rams = factory.getPlugins ("ram");
		if (rams.size () > 0) {
			this-> _ramPlugin = rams[0];
			if (rams.size () > 1) LOG_WARN ("Core 'dumper' can manage only one RAM plugin.");
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

	void Dumper::configureCpuFrequencies () {
		auto root = "/sys/devices/system/cpu/";
		for (uint32_t i = 0 ; ; i++) {
			auto cpu = utils::join_path (root, "cpu" + std::to_string (i));
			auto freq = utils::join_path (cpu, "cpufreq/scaling_cur_freq");
			if (utils::file_exists (freq)) {
				this-> _cpuFreqFds.push_back (fopen (freq.c_str (), "r"));
			} else break;
		}

		this-> _cpuFreqs.resize (this-> _cpuFreqFds.size ());
	}
    
    void Dumper::compute () {
		if (this-> _needUpdate) {
			this-> configureCgroups ();
			this-> _needUpdate = false;
		}
	
		for (auto & it : this-> _pollFunctions) {
			it ();
		}
	
		this-> pollPerfEvents ();
	
		this-> computeCpuEnergy ();
		this-> computeRamEnergy ();
		this-> computeGpuEnergy ();
		this-> pollCpuFrequencies ();

		this-> writeResults ();
    }

    void Dumper::mountResultDir () {
		auto mntType = utils::get_mount_type (this-> _outputDir);
		if (mntType == "tmpfs") {
			umount (this-> _outputDir.c_str ());
		}
	
		if (!utils::file_exists (this-> _outputDir)) {
			utils::create_directory (this-> _outputDir, true);
		}

		int rc = mount ("tmpfs", this-> _outputDir.c_str (), "tmpfs", 0, "size=512M,uid=0,gid=0,mode=777");
		if (rc != 0) {
			LOG_WARN ("Failed to mount result dir ", this-> _outputDir, " in tmpfs.");
		} else {
			LOG_INFO ("Result dir ", this-> _outputDir, " is mounted in tmpfs");
		}
    }        
       
    void Dumper::createResultsFiles() {
		utils::create_directory (this-> _outputDir, true);
		this-> _resultsPerfOs = std::ofstream(utils::join_path (this-> _outputDir, "cgroups.csv"), std::ios::out);
		this-> _resultsPerfOs << "TIMESTAMP;CGROUP";
		for (auto perfEvent: this-> _perfEvents) {
			this-> _resultsPerfOs << ";" << perfEvent;
		}
		this-> _resultsPerfOs << std::endl;

		this-> _resultsEnergyOs = std::ofstream(utils::join_path (this-> _outputDir, "energy.csv"), std::ios::out);
		this-> _resultsEnergyOs << "TIMESTAMP;CPU;RAM;GPU";
		for (uint32_t i = 0 ; i < this-> _cpuFreqFds.size () ; i++) {
			this-> _resultsEnergyOs << ";FREQ" << i;
		}
		this-> _resultsEnergyOs << std::endl;
    }

    void Dumper::writeResults() {
		struct timeval start;
		gettimeofday(&start, NULL);

		for (int i = 0; i < this-> _cgroupList.size (); i++) {
			this-> _resultsPerfOs << start.tv_sec << "." << start.tv_usec << ";";

			if (this-> _cgroupList[i].getName () == "") {
				this-> _resultsPerfOs << "#SYSTEM";
			} else {
				this-> _resultsPerfOs << this-> _cgroupList[i].getName ();
			}

			for(auto perfValue: this-> _perfEventValues[i]) {
				this-> _resultsPerfOs << ";" << perfValue;
			}
			this-> _resultsPerfOs << std::endl;
		}

		this-> _resultsEnergyOs << start.tv_sec << "." << start.tv_usec << ";" << this-> _cpuEnergy << ";" << this-> _ramEnergy << ";" << this-> _gpuEnergy;
		for (auto & it : this-> _cpuFreqs) {
			this-> _resultsEnergyOs << ";" << it;
		}
		this-> _resultsEnergyOs << std::endl;
    }


    void Dumper::pollPerfEvents () {
		for (uint64_t i = 0 ; i < this-> _cgroupList.size () ; i++) {
			this-> _cgroupWatchers[i].poll (this-> _perfEventValues[i]);
		}
    }

	void Dumper::pollCpuFrequencies () {
		for (uint64_t i = 0 ; i < this-> _cpuFreqFds.size () ; i++) {
			fseek (this-> _cpuFreqFds [i], 0, SEEK_SET);
			if (fscanf (this-> _cpuFreqFds [i], "%ld", &this-> _cpuFreqs [i]) == 0) {
				LOG_WARN ("Failed to read frequency")
			}
			fflush (this-> _cpuFreqFds [i]);
		}
	}

    void Dumper::computeGpuEnergy () {
		for (uint64_t i = 0 ; i < this-> _gpuGet.size () ; i++) {
			this-> _gpuGet[i](this-> _gpuEnergyCache[i].data ());
			for (uint64_t device = 0 ; device < this-> _gpuEnergyCache[i].size () ; device++) {
				this-> _gpuEnergy += this-> _gpuEnergyCache[i][device];
			}
		}
    }

    void Dumper::computeRamEnergy () {
		if (this-> _ramGet != nullptr) {
			this-> _ramEnergy += this-> _ramGet ();
		}
    }

    void Dumper::computeCpuEnergy () {
		if (this-> _cpuGet != nullptr) {
			this-> _cpuEnergy += this-> _cpuGet ();
		}
    }
  
    void Dumper::configureCgroups () {
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
				watchers.back ().reconfigure (this-> _perfEvents);
				cache.push_back (std::move (this-> _perfEventValues [i]));
				lst.erase (this-> _cgroupList[i]);
			} else {
				this-> _cgroupWatchers[i].dispose ();
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
			pw.configure (this-> _perfEvents);
			this-> _cgroupWatchers.push_back (std::move (pw));
			this-> _perfEventValues.push_back({});
			this-> _perfEventValues.back ().resize (this-> _perfEvents.size ());
			this-> _cgroupList.push_back (it);
		}

		this-> _notif.configure (this-> _cgroupFile, rules);
		this-> _notif.start ();
    }

    
    std::vector <std::string> Dumper::readCgroupRules () const {       
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

    void Dumper::dispose () {
		this-> _cgroupWatchers.clear ();
		auto mntType = utils::get_mount_type (this-> _outputDir);
		if (mntType == "tmpfs") {
			umount (this-> _outputDir.c_str ());
			utils::remove (this-> _outputDir);
		}
	
		LOG_INFO ("Disposing dumper core.");
    }
    

    
}
