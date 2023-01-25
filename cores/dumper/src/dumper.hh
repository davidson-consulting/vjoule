#pragma once

#define __PROJECT__ "DUMPER"
#define __PLUGIN_VERSION__ "1.0"

#include <common/_.hh>
#include <vector>
#include <set>
#include <fstream>

#include "notif.hh"

namespace dumper {    
    class Dumper {
    private:
	
	// directory where results should be written
	std::string _outputDir;

	// The list of cgroup to watch
	std::vector <common::cgroup::Cgroup> _cgroupList;

	// The list of perf events to monitor
	std::vector <std::string> _perfEvents;

	// The watchers of the cgroup perf event counters
	std::vector <common::perf::PerfEventWatcher> _cgroupWatchers;

	// The values of the perf event counters of the last poll
	std::vector <std::vector <uint64_t> > _perfEventValues;

	// The file listing cgroup to watch
	std::string _cgroupFile;

	// The current energy values
	float _cpuEnergy;
	float _ramEnergy;
	float _gpuEnergy;

	// The result files
	std::ofstream _resultsPerfOs;
	std::ofstream _resultsEnergyOs;

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
	
	// The list of plugins used by the dumper
	// We use a set because we need to poll the plugins only one time, even if they are used for different metrics
	std::set <common::plugin::PluginPollFunc_t> _pollFunctions;
	
    public :

	/**
	 * Create an empty dumper
	 */
	Dumper ();

	/**
	 * Configure the dumper
	 * @params: 
	 *    - cfg: the configuration of the dumper
	 */
	bool configure (const common::utils::config::dict & cfg, common::plugin::Factory & factory);

	/**
	 * Poll the energy consumption of the different monitored components, the perf events and output
	 * results in the CSV result files 
	 */
	void compute ();

	/**
	 * Dispose the divider and all its handles
	 */
	void dispose ();

    private: 
	
	/**
	 * Configure the cgroups watched by the dumper
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

	/*
	 * Create results csv files
	 */
	void createResultsFiles();

	/**
	 * Write the results
	 */
	void writeResults ();
	
    };
    
}
