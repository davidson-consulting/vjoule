#pragma once

#include <common/_.hh>
#include <common/foreign/CL11.hpp>
#include <sensor/perf/watch.hh>
#include <common/cgroup/_.hh>

namespace sensor {


    typedef void (*FormulaComputeFunc_t)(const std::vector <common::cgroup::Cgroup> & cgroups,
					 const std::vector <std::vector<uint64_t> > & perfCounters,
					 common::plugin::Factory & factory);
    
    class Sensor {
    private:

	common::utils::Logger _logger;
	
	// The path of the configuration file
	std::string _cfgPath;

	// The file listing the cgroup to watch
	std::string _cgroupFile;

	// The time to wait between to dumping/formula
	float _freq;

	// if true dump the plugins and formula to csv files
	bool _dump = false;
	
	// The list of events used by the formula
	std::vector <std::string> _events;

	// The list of cgroup to watch
	std::vector <common::cgroup::Cgroup> _cgroupList;
	
	// The watchers of the cgroup perf event counters
	std::vector <perf::PerfEventWatcher> _cgroupWatchers;

	// The values of the perf event counters of the last poll
	std::vector <std::vector <uint64_t> > _perfEventValues;

    private :
	
	// The factory of plugins
	common::plugin::Factory _factory;

	// The list of dumping files for dumping plugin
	std::unordered_map <std::string, std::ofstream> _dumpingFiles;

    private: 
	
	// The formula plugin
	common::plugin::Plugin* _formula;
	
	FormulaComputeFunc_t _formulaCompute;

	FormulaComputeFunc_t _formulaDump = nullptr;
	
    private:

	// The number of parameters passed to the program
	int _argc;

	// The parameters passed to the program
	char ** _argv;

	// The app option parser
	CLI::App _app;

	CLI::Option * _cfgPathOpt;
	
	CLI::Option * _cgroupPathOpt;

	CLI::Option * _logPathOpt;

	CLI::Option * _logLvlOpt;

	CLI::Option * _freqOpt;
	
    public:

	/**
	 * @params: 
	 *    - argc: command line option size
	 *    - argv: command line option content
	 */
	Sensor (int argc, char ** argv);
	
	/**
	 * Run the sensor (infinite loop)
	 */
	void run () ;
	
    private :

	/**
	 * Init the option of the app (command line)
	 */
	void initAppOptions ();

	/**
	 * Configure the sensor from a configuration file
	 */
	void configure (const common::utils::config::dict & config);	

	/**
	 * Configure the formula of the sensor
	 */
	void configureFormula (const common::utils::config::dict & config);

	/**
	 * Configure the options of the command line
	 */
	void configureOptions ();

	/**
	 * Configure the perf event watchers of the sensor
	 */
	void configurePerfEventWatchers ();
	
	/**
	 * Configure the plugin with a given configuration
	 */
	void configurePlugin (const std::string & kind, const common::utils::config::dict & config);

	/**
	 * Read the list of cgroup from the cgroup file
	 */
	std::vector <common::cgroup::Cgroup> readCgroupList () ;

	/**
	 * Poll all loaded plugins
	 */
	void pollPlugins ();

	/**
	 * Poll the perf events of the cgroup
	 */
	void pollPerfEvents ();
	
	/**
	 * Call the formula 
	 */
	void callFormula ();

	/**
	 * Dump all the loaded plugins to csv
	 */
	void dumpPlugins ();

	/**
	 * Dump the formula to csv
	 */
	void dumpFormula ();
	
	/**
	 * Dispose all loaded plugins/formula of the sensor
	 */
	void dispose ();
	
    };
    
}
