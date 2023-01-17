#pragma once

#define __PROJECT__ "DIVIDER"

#include <common/_.hh>
#include <vector>
#include <set>

#include "notif.hh"

namespace divider {

    struct CgroupResult {
	float cpu;
	float ram;
	float gpu;
	
	FILE * cpuFd;
	FILE * ramFd;
	FILE * gpuFd;
    };    

    class Divider {
    private:

	// The list of cgroup to watch
	std::vector <common::cgroup::Cgroup> _cgroupList;
	
	// The watchers of the cgroup perf event counters
	std::vector <common::perf::PerfEventWatcher> _cgroupWatchers;

	// The values of the perf event counters of the last poll
	std::vector <std::vector <uint64_t> > _perfEventValues;

	// The file listing cgroup to watch
	std::string _cgroupFile;

	// If true cgroups have changed
	bool _needUpdate = false;

	// The notifier used to check cgroup modifications
	Notifier _notif;
	
    private : 	
	
	// The list of gpu plugins
	std::vector <common::plugin::Plugin*> _gpuPlugins;

	// The list of get power function of the gpu plugins
	std::vector <common::plugin::GpuGetEnergy_t> _gpuGet;

	// The list of get perf percentage usage function of the gpu plugins
	std::vector <common::plugin::GpuGetDeviceUsage_t> _gpuPerfEvents;
	
	// The cache for power consumption reading on gpu
	std::vector <std::vector <float> > _gpuEnergyCache;
	
    private : 
	
	// The plugin for cpu consumption
	common::plugin::Plugin* _cpuPlugin;

	// The get power function of the cpu plugin
	common::plugin::CpuGetEnergy_t _cpuGet = nullptr;
	
    private: 
	
	// The plugin for ram consumption
	common::plugin::Plugin* _ramPlugin;

	// The get power function of the ram plugin
	common::plugin::RamGetEnergy_t _ramGet = nullptr;
	
    private : 
	
	// The list of plugins used by the divider
	// We use a set because we need to poll the plugins only one time, even if they are used for different metrics
	std::set <common::plugin::PluginPollFunc_t> _pollFunctions;

	// The result of the watched cgroups
	std::unordered_map <std::string, CgroupResult> _results;
	
    public :

	/**
	 * Create an empty divider
	 */
	Divider ();

	/**
	 * Configure the divider
	 * @params: 
	 *    - cfg: the configuration of the divider
	 */
	bool configure (const common::utils::config::dict & cfg, common::plugin::Factory & factory);

	/**
	 * Compute the consumption of the different cgroup using the factory plugins
	 * @params: 
	 *    - factory: the list of loaded plugins
	 */
	void compute ();

	/**
	 * Dispose the divider and all its handles
	 */
	void dispose ();

    private: 
	
	/**
	 * Configure the cgroups watched by the divider
	 */
	void configureCgroups ();

	/**
	 * Configure the gpu plugins
	 */
	bool configureGpuPlugins (common::plugin::Factory & factory);

	/**
	 * Configure the cpu plugin
	 */
	bool configureCpuPlugin (common::plugin::Factory & factory);

	/**
	 * Configure the ram plugin
	 */
	bool configureRamPlugin (common::plugin::Factory & factory);
	
	/**
	 * Retreive the list of cgroup to watch
	 */
	std::vector <std::string> readCgroupRules () const;

    private: 

	/**
	 * Poll the performance events
	 */
	void pollPerfEvents ();

	/**
	 * Compute the cpu power consumption per cgroup
	 */
	void computeCpuEnergy ();

	/**
	 * Compute the ram power consumption per cgroup
	 */
	void computeRamEnergy ();

	/**
	 * Compute the gpu power consumption per cgroup
	 */
	void computeGpuEnergy ();

	/**
	 * Function called by notifier when a cgroup modification is required
	 */
	void onCgroupUpdate ();

	/**
	 * Create the directory to hold the result of a given cgroup
	 */
	void createResultDirectory (const common::cgroup::Cgroup & c);

	/**
	 * Remove the directory to hold the result of a given cgroup
	 */
	void removeResultDirectory (const common::cgroup::Cgroup & c);

	/**
	 * Write the consumption results
	 */
	void writeConsumption ();
	
    };
    
}
