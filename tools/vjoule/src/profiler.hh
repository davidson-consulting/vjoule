#pragma once

#include "command.hh"
#include <common/_.hh>

namespace tools::vjoule {

    struct ResultRow {
	float cpu;
	float ram;
    };

    /**
     * Compute pi
     */
    double computePi (uint64_t nbIter);

    class Profiler {
    private : 

	// The command line used to configure the profiler
	CommandLine _cmd;

	// The output directory for vjoule results
	std::string _vjouleDir;

	// The path of the configuration file
	std::string _cfgPath;

	// The number of iteration of each load
	uint64_t _nbIter;

	// 
	double _allRes = 0;
	
    public: 

	Profiler (const CommandLine & cmd);

	/**
	 * Run the profiler
	 */
	void run ();

    private:

	/**
	 * Print the result of the profiling to stdout
	 */
	void printResult (const std::vector <ResultRow> & res) const;

	/**
	 * Wait for the service to write a new result
	 */
	void waitServiceIteration () const;

	/**
	 * Read the current consumption 
	 */
	void readConsumption (float &cpu, float &ram) const;

	/**
	 * Run the load on nb cores
	 */
	void runLoad (uint64_t nb);

	/**
	 * Create the configuration file
	 */
	void createConfiguration () const;

	/**
	 * Run the compute pi load
	 */
	void stress (common::concurrency::thread);
	
    };
    

}
