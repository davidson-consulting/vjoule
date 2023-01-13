#pragma once

#include <common/net/packet.hh>
#include <common/perf/rapl_utils.hh>
#include <common/utils/_.hh>
#include <common/concurrency/_.hh>
#include <common/net/_.hh>

namespace sensor::perf {

    enum class RaplReaderMode : int {
	BARE_METAL = 0,
	TCP,
	NFS,
	NONE
    };      
    
    
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

	/**
	 * List of elements used in NFS mode
	 */
	struct Nfs {
	    // The nfs local path 
	    std::string localPath;

	    // The speed of the polling (check if file was updated)
	    double pollSpeed;
	    
	    // the path to the header file
	    std::string headPath;

	    // the path to the system metric path
	    std::string valuePath;

	    // The path to the power limit file
	    std::string powerLimitPath;

	    // The path to the frequency file 
	    std::string freqPath;

	    // The timestamp of the last time package file was modified
	    unsigned long lastPollSec = 0;

	    // The timestamp of the last time package file was modified
	    unsigned long lastPollnSec = 0;

	    // The timer used to wait the correct amount of time between report writing
	    common::concurrency::timer readTimer;
	    
	    // The values of the last read
	    common::perf::PackageContent lastValue = {};
	};

	/**
	 * List of elements used in TCP mode
	 */
	struct Tcp {
	    // The port of the host tcp_vm service
	    int hostPort;
	};	
	
    private : 

	// Bare metal mode
	BareMetal _bare;

	// Nfs mode
	Nfs _nfs;

	// Tcp mode
	Tcp _tcp;

	// The cpu information of the machine (host if in VM)
	// common::utils::CpuInfo cpuInfo;
	    	    	
	// The mode of reading
	RaplReaderMode _mode;
       	
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
	void configure (const common::utils::config::dict & cfg);

	/**
	 * Poll the values of rapl to packet
	 * @warning: in TCP or NFS mode, this function waits for the next values, in BARE_METAL immediately reads and return
	 */       
	void poll (common::net::Packet & packet);

	/**
	 * @returns: the mode of reading
	 */
	RaplReaderMode getMode () const;

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
	
	/**
	 * ================================================================================
	 * ================================================================================
	 * =========================        TCP CONFIGURATION         =====================
	 * ================================================================================
	 * ================================================================================
	 */
	
	/**
	 * Connect the sensor to tcp reading
	 * @returns: true if succeed
	 */
	bool connectToTcp ();


	/**
	 * Poll the rapl data in tcp mode
	 */
	void pollTcp (common::net::Packet & pack);

	/**
	 * ================================================================================
	 * ================================================================================
	 * =========================        NFS CONFIGURATION         =====================
	 * ================================================================================
	 * ================================================================================
	 */
	
	/**
	 * Mount the formula of the host
	 * @returns: true if succeed
	 */
	bool mountHostFormula ();
	
	/**
	 * Poll the rapl data in tcp mode
	 */
	void pollNfs (common::net::Packet & pack);

	/**
	 * @warning: should only be used when the nfs has been disconnected for a reason or another
	 * @warning: infinite loop if the nfs never reconnects
	 */
	void waitNfsAvailability ();

	/**
	 * Poll the nfs files until there is a modification to read
	 */
	void waitNfsModification ();
	
	/**
	 * Open the nfs files for rapl reading
	 */
	bool checkNfsFiles ();
	
	/**
	 * Read cpu informations from nfs (e.g. /vjoule/cpuinfo)
	 */
	void readCpuInfoNfs ();
	
	/**
	 * Dismount the nfs dir if mounted
	 */
	void dismountNfs ();

	/**
	 * Mount the nfs directory from hostPath to a local path
	 * @returns: true if succeed
	 */
	bool mountNfs (const std::string & hostIp, const std::string & hostPath);
	
	/**
	 * Find the ips of the machine
	 */
	std::vector<std::string> findVMIps () const;
	
    };    

}
