#pragma once

#include <common/net/packet.hh>
#include <common/perf/rapl_utils.hh>
#include <common/utils/_.hh>
#include <common/concurrency/_.hh>
#include <common/net/_.hh>

namespace tools::vjoule {  
    /**
     * Class reading rapl values
     */
    class RaplReader {

	/**
	 * List of elements used in BARE METAL mode
	 */
	struct BareMetal {
	    // The content of the last read
	    std::vector<common::perf::PackageContent> lastValues;
	    
	    // The package units
	    std::vector <common::perf::PackageUnits> packageUnits;

	    // The package map (package ids)
	    std::vector <int> packageMap;

	    // The msr file descriptors (BARE_METAL)
	    std::vector <int> fds;

	    // The temperature file
	    std::string tempFile;

	    // The files of cpu frequency
	    std::vector <std::string> freqFiles;
	    
	    // The list of available event on the core arch
	    common::perf::EventAvail raplAvail;

	    // The model of the cpu
	    int cpuModel;

	    // The number of package
	    int totalPackages;

	    // The number of cores
	    int totalCores;
	};
	
    private : 

	// Bare metal mode
	BareMetal _bare;

	// The cpu information of the machine (host if in VM)
	// common::utils::CpuInfo cpuInfo;
       	
    private :

	RaplReader (const RaplReader& other);

	void operator=(const RaplReader& other);
	
    public :

	/**
	 * Create an empty unconfigured rapl reader
	 */
	RaplReader ();

	/**
	 * Poll the values of rapl to packet
	 * @warning: in TCP or NFS mode, this function waits for the next values, in BARE_METAL immediately reads and return
	 */       
	void poll (common::net::Packet & packet);

	/**
	 * @returns: the list of system events sent by the bridge
	 * @warning: only available in NFS mode
	 */
	std::vector <std::string> readSystemEvents () const;

	/**
	 * @returns: the power limits sent by the bridge
	 * @warning: only available in NFS mode
	 */
	void readPowerLimits (float & PMIN, float & PMOY, float & PMAX) const;

	
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
	 * Read cpu informations from bare metal infos (e.g. /proc/cpuinfo)
	 */
	void readCpuInfoBareMetal ();

	/**
	 * Find the file reading the cpu temperature
	 */
	void openCpuTempFile ();

	/**
	 * Find the files reading the frequency of the cpu cores
	 */
	void openFrequencyFiles ();
	
	/**
	 * Poll the rapl data in bare metal mode
	 */
	void pollBareMetal (common::net::Packet & pack);
    };    

}
