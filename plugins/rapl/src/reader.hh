#pragma once

#ifndef __PROJECT__
#define __PROJECT__ "RAPL"
#endif

#define __PLUGIN_VERSION__ "1.0"

#include "utils.hh"

#include <common/utils/_.hh>

namespace rapl {
    
    /**
     * Class reading rapl values
     */
    class RaplReader {
	
	// The cache of rapl values for read_package_values
	std::vector<rapl::PackageCache> _cache;
	    
	// The package units
	std::vector <rapl::PackageUnits> _packageUnits;

	// The package map (package ids)
	std::vector <int> _packageMap;

	// The msr file descriptors (BARE_METAL)
	std::vector <int> _fds;

	// The temperature file
	std::string _tempFile;

	// The files of cpu frequency
	std::vector <std::string> _freqFiles;
	    
	// The list of available event on the core arch
	rapl::EventAvail _raplAvail;

	// The model of the cpu
	int _cpuModel;

	// The number of package
	int _totalPackages;

	// The number of cores
	int _totalCores;

	// true iif pp1 is available
	bool _gpuAvail = false;
	

	double _energy_pp0;
	double _energy_pp1;
	double _energy_pkg;
	double _energy_psys;
	double _energy_dram;
	
    private :

	RaplReader (const RaplReader& other);

	void operator=(const RaplReader& other);
	
    public :

	/**
	 * Create an empty unconfigured rapl reader
	 */
	RaplReader ();

	/**
	 * Configure the rapl reader
	 */
	bool configure ();

	/**
	 * Poll the values of rapl to packet
	 * @warning: in TCP or NFS mode, this function waits for the next values, in BARE_METAL immediately reads and return
	 */       
	void poll ();

	/**
	 * @returns: the consumption of package
	 */
	float getCpuEnergy () const;

	/**
	 * @returns: the consumption of ram (0 might mean it is not available)
	 */	
	float getRamEnergy () const;

	/**
	 * @returns: the consumption of pp1 (0 might mean it is not available)
	 */
	float getGpuEnergy () const;

	/**
	 * @returns: the number of devices
	 */
	uint32_t getGpuNbDevices () const;
	
	/**
	 * Dispose the rapl reader
	 */
	void dispose ();

	/**
	 * call this-> dispose ()
	 */
	~RaplReader ();


    private :


	/**
	 * ================================================================================
	 * ================================================================================
	 * ======================        BARE METAL CONFIGURATION         =================
	 * ================================================================================
	 * ================================================================================
	 */
	
	/**
	 * Open the msr file descriptors
	 * @returns: true if succeed
	 */
	bool openMsrFiles ();

	/**
	 * Read cpu informations 
	 */
	void readCpuInfo ();

	/**
	 * Find the file reading the cpu temperature
	 */
	void openCpuTempFile ();

	/**
	 * Find the files reading the frequency of the cpu cores
	 */
	void openFrequencyFiles ();
	
    };    

}
