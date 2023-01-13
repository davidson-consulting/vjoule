#include <sensor/perf/rapl.hh>
#include <common/perf/rapl_utils.hh>
#include <common/utils/log.hh>
#include <common/utils/files.hh>
#include <common/concurrency/proc.hh>
#include <common/concurrency/timer.hh>
#include <common/net/ports.hh>

#include <filesystem>
#include <netdb.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sysinfo.h>

using namespace common;
using namespace common::utils;
using namespace common::perf;
namespace fs = std::filesystem;

namespace sensor::perf {

    RaplReader::RaplReader () {
    }

    RaplReaderMode RaplReader::getMode () const {
	return this-> _mode;
    }	
    
    /**
     * ================================================================================
     * ================================================================================
     * =========================        CONFIGURATION         =========================
     * ================================================================================
     * ================================================================================
     */

    void RaplReader::configure (const utils::config::dict & cfg) {
	this-> _tcp.hostPort = cfg.getOr<int> ("host-port", TCP_VM_DEFAULT_PORT);
	this-> _nfs.pollSpeed = 1 / cfg.getOr<double> ("poll-freq", 3);

	bool dontPoll = true;
	if (cfg.has<bool> ("no-energy")) {	    
	    if (cfg.get<bool> ("no-energy")) {
		dontPoll = true;
		this-> _mode == RaplReaderMode::NONE;
	    }
	}
	
	if (this-> openMsrFiles ()) {
	    this-> openCpuTempFile ();
	    this-> openFrequencyFiles ();
	    this-> _mode = RaplReaderMode::BARE_METAL;
	} else if (!dontPoll) { // don't poll if not bare metal and no-energy
	    if (this-> connectToTcp ()) {
		this-> _mode = RaplReaderMode::TCP;
	    } else {
		this-> waitNfsAvailability ();
		this-> _mode = RaplReaderMode::NFS;	    		    
	    }
	}
    }

    /**
     * ================================================================================
     * ================================================================================
     * =========================        BARE METAL CFG         ========================
     * ================================================================================
     * ================================================================================
     */
    
    bool RaplReader::openMsrFiles () {
	utils::Logger::globalInstance ().info ("RaplReader : try to configure in bare metal mode.");
	this-> _bare.cpuModel = detect_cpu ();
	this-> _bare.packageMap = detect_packages (this-> _bare.totalPackages, this-> _bare.totalCores);	
	this-> _bare.raplAvail = detect_avail (0, this-> _bare.cpuModel);
	
	try {
	    for (auto & it: this-> _bare.packageMap) {
		this-> _bare.fds.push_back (open_msr (it));

		auto packageUnit = read_package_unit (this-> _bare.fds.back (), this-> _bare.raplAvail);
		this-> _bare.packageUnits.push_back (packageUnit);

		utils::Logger::globalInstance ().info ("Power units[", it, "] = ", packageUnit.powerUnits, "W");
		utils::Logger::globalInstance ().info ("CPU energy units[", it, "] = ",  packageUnit.cpuEnergyUnits, "J");
		utils::Logger::globalInstance ().info ("DRAM energy units[", it, "] = ", packageUnit.dramEnergyUnits, "J");
		utils::Logger::globalInstance ().info ("Time units[", it, "] = ", packageUnit.timeUnits, "s");
		this-> _bare.lastValues.push_back (read_package_values (this-> _bare.fds.back (), this-> _bare.raplAvail, packageUnit));
	    }

	    utils::Logger::globalInstance ().info ("RaplReader : bare metal mode is available.");
	    return true;
	} catch (...) {
	    this-> _mode = RaplReaderMode::BARE_METAL;
	    this-> dispose (); // disposing because some elements where opened even if they failed to provide data

	    utils::Logger::globalInstance ().warn ("RaplReader : failed to configure in bare metal mode.");
	    return false;
	}
    }

    void RaplReader::openCpuTempFile () {
	utils::Logger::globalInstance ().info ("RaplReader : find core temperature file");
	for (int i = 0 ; ; i++) {
	    std::stringstream filename;
	    filename << "/sys/class/thermal/thermal_zone" << i << "/type";
	    std::ifstream type (filename.str ().c_str ());
	    if (type.is_open ()) {
		std::string value;
		type >> value;
		if (value == "x86_pkg_temp") {
		    std::stringstream tempname;
		    tempname << "/sys/class/thermal/thermal_zone" << i << "/temp";
		    this-> _bare.tempFile = tempname.str ();
		    utils::Logger::globalInstance ().info ("Temperature file found : ", tempname.str ());
		    return;		    
		}
	    } else break;
	}

	utils::Logger::globalInstance ().error ("Failed to find cpu temperature file");
    }

    void RaplReader::openFrequencyFiles () {
	utils::Logger::globalInstance ().info ("RaplReader : find cpu frequency files");
	for (int i = 0 ; ; i++) {
	    std::stringstream filename;
	    filename << "/sys/devices/system/cpu/cpu" << i << "/cpufreq/scaling_cur_freq";
	    std::ifstream f (filename.str ().c_str ());
	    if (f.is_open ()) {
		this-> _bare.freqFiles.push_back (filename.str ());
		utils::Logger::globalInstance ().info ("Frequency file found : ", filename.str ());
	    } else break;
	}
    }

    
    /**
     * ================================================================================
     * ================================================================================
     * =========================        TCP CONFIGURATION         =====================
     * ================================================================================
     * ================================================================================
     */

    bool RaplReader::connectToTcp () {
	utils::Logger::globalInstance ().info ("RaplReader : try to configure in tcp mode.");

	//TODO

	utils::Logger::globalInstance ().warn ("RaplReader : failed to configure in tcp mode.");

	this-> _mode = RaplReaderMode::TCP;
	this-> dispose ();
	
	return false;
    }

    
    /**
     * ================================================================================
     * ================================================================================
     * =========================        NFS CONFIGURATION         =====================
     * ================================================================================
     * ================================================================================
     */

    void RaplReader::waitNfsAvailability () {
	common::concurrency::timer t;
	while (true) {
	    if (this-> mountHostFormula ()) {
		return;
	    } else {
		utils::Logger::globalInstance ().error ("Host formula disconnected. sleep 1s...");
		t.sleep (1);
	    }
	}
    }

    
    bool RaplReader::mountHostFormula () {
	this-> _nfs.localPath = utils::join_path (VJOULE_DIR, "sensor/rapl");
	this-> dismountNfs (); // we dismount it by security
	utils::Logger::globalInstance ().info ("RaplReader : try to configure in nfs mode.");
	
	auto ips = this-> findVMIps ();
	for (auto & ip : ips) {
	    auto hostIp = ip.substr (0, ip.rfind (".")) + ".1";
	    auto proc = concurrency::SubProcess ("showmount", {"--no-headers", "-e", hostIp}, ".");
	    proc.start ();
	    if (proc.wait () == 0) {
		std::stringstream ss (proc.stdout ().read ());
		std::string content;
		while (std::getline (ss, content, '\n')) {
		    if (content.length () != 0) {
			auto path = content.substr (0, content.find (" "));
			auto ip = content.substr (content.find (" ") + 1);
			fs::create_directories (this-> _nfs.localPath);
			if (this-> mountNfs (hostIp, path)) {
			    if (this-> checkNfsFiles ()) {
				utils::Logger::globalInstance ().info ("RaplReader : nfs mode is available.");
				return true;
			    } else this-> dismountNfs ();
			}
		    }
		}
	    }
	}

	this-> _mode = RaplReaderMode::NFS;
	this-> dispose ();
	
	utils::Logger::globalInstance ().warn ("RaplReader : failed to configure in nfs mode.");
	return false;
    }


    bool RaplReader::checkNfsFiles () {
	this-> _nfs.powerLimitPath = utils::join_path (this-> _nfs.localPath, "power_limits");
	auto packageFd = fopen (this-> _nfs.powerLimitPath.c_str (), "r");
	if (packageFd == nullptr) {
	    utils::Logger::globalInstance ().error ("No power_limits file found in nfs directory.");
	    return false;
	} else fclose (packageFd);

	this-> _nfs.headPath = utils::join_path (this-> _nfs.localPath, "perf_counter_header");
	auto headerFd = fopen (this-> _nfs.headPath.c_str (), "r");
	if (headerFd == nullptr) {
	    utils::Logger::globalInstance ().error ("No header is available in nfs directory.");
	    return false;
	} else fclose (headerFd);
	
	this-> _nfs.valuePath = utils::join_path (this-> _nfs.localPath, "perf_counter_values");
	auto valueFd = fopen (this-> _nfs.valuePath.c_str (), "r");
	if (valueFd == nullptr) {
	    utils::Logger::globalInstance ().error ("Values are not available in nfs directory");
	    return false;
	} else fclose (valueFd);

	this-> _nfs.freqPath = utils::join_path (this-> _nfs.localPath, "freq"); // optional file

	this-> _nfs.pollSpeed = 0;
	
	return true;
    }

    std::vector <std::string> RaplReader::readSystemEvents () const {
	std::ifstream file (this-> _nfs.headPath);
	std::string line;
	std::vector <std::string> events;
	
	while (std::getline (file, line, ';')) {
	    events.push_back (utils::strip_string (line));
	}
	file.close ();

	return events;
    }

    void RaplReader::readPowerLimits (float & PMIN, float &PMOY, float &PMAX) const {
	std::ifstream file (this-> _nfs.powerLimitPath);
	char ig;
	
	file >> PMIN >> ig >> PMOY >> ig >> PMAX;
	file.close ();
    }
       
    std::vector<std::string> RaplReader::findVMIps () const {
	struct ifaddrs * addresses;
	if (getifaddrs (&addresses) == -1) {
	    utils::Logger::globalInstance ().error ("Machine has no ip interfaces?");
	    throw std::runtime_error ("");
	}

	struct ifaddrs* address = addresses;
	std::vector <std::string> vmIps;
	while (address) {
	    int family = address->ifa_addr->sa_family;
	    if (family == AF_INET && std::string (address->ifa_name) != "virbr0")
	    {
		char ap[100];
		const int family_size = family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
		getnameinfo(address->ifa_addr,family_size, ap, sizeof(ap), 0, 0, NI_NUMERICHOST);
		if (strcmp (ap, "127.0.0.1") != 0) {
		    vmIps.push_back (std::string (ap));
		}
	    }
	    address = address->ifa_next;
	}
	    
	freeifaddrs(addresses);	
	return vmIps;
    }

    void RaplReader::dismountNfs () {
	auto proc = concurrency::SubProcess ("umount", {this-> _nfs.localPath}, ".");
	proc.start ();

	auto code = proc.wait ();
	if (code != 0 && code != 8192) {
	    utils::Logger::globalInstance ().error ("Failed to umount old vjoule dir : ", proc.stderr ().read ());
	    throw std::runtime_error ("");
	}
    }

    bool RaplReader::mountNfs (const std::string & hostIp, const std::string & hostPath) {
	auto proc = concurrency::SubProcess ("mount", {"-t", "nfs", hostIp + ":" + hostPath, this-> _nfs.localPath}, ".");
	proc.start ();
	if (proc.wait () == 0) {	    
	    utils::Logger::globalInstance ().info ("Nfs vjoule mounted to : ", this-> _nfs.localPath);
	    return true;
	} else {
	    utils::Logger::globalInstance ().error ("Failed to mount nfs : ", proc.stderr ().read ());
	    return false;
	}
    }


    /**
     * ================================================================================
     * ================================================================================
     * =========================        RESULT POLLING         ========================
     * ================================================================================
     * ================================================================================
     */

    
    void RaplReader::poll (common::net::Packet & packet) {
	packet.energy_pp0 = 0;
	packet.energy_pp1 = 0;
	packet.energy_pkg = 0;
	packet.energy_dram = 0;
	packet.energy_psys = 0;

	if (this-> _mode == RaplReaderMode::NONE) return;	
	if (this-> _mode == RaplReaderMode::BARE_METAL) { // bare metal sensor
	    this-> pollBareMetal (packet);
	} else if (this-> _mode == RaplReaderMode::TCP) { 
	    this-> pollTcp (packet);
	} else {
	    this-> pollNfs (packet);
	}
    }


    void RaplReader::pollBareMetal (common::net::Packet & packet) {
	for (size_t i = 0 ; i < this-> _bare.fds.size () ; i++) {
	    auto values = read_package_values (this-> _bare.fds[i], this-> _bare.raplAvail, this-> _bare.packageUnits[i]);
	    packet.energy_pp0 += (values.pp0 - this-> _bare.lastValues[i].pp0);
	    packet.energy_pp1 += (values.pp1 - this-> _bare.lastValues[i].pp1);
	    packet.energy_dram += (values.dram - this-> _bare.lastValues[i].dram);
	    packet.energy_pkg += (values.package - this-> _bare.lastValues[i].package);
	    packet.energy_psys += (values.psys - this-> _bare.lastValues[i].psys);
	    
	    this-> _bare.lastValues [i] = values;
	}

	if (this-> _bare.tempFile != "") {
	    std::ifstream f (this-> _bare.tempFile);
	    f >> packet.core_temp;
	    f.close ();
	}

	
	packet.cpuFreq.resize (this-> _bare.freqFiles.size ());
	for (int i = 0 ; i < this-> _bare.freqFiles.size () ; i++) {
	    std::ifstream f (this-> _bare.freqFiles[i]);
	    f >> packet.cpuFreq[i];
	    f.close ();
	}
    }

    void RaplReader::pollTcp (common::net::Packet & packet) {}
    

    void RaplReader::pollNfs (common::net::Packet & packet) {	
	this-> waitNfsModification ();
	
	if (packet.globalMetrics.size () == 0) return;
	
	std::ifstream file (this-> _nfs.valuePath);
	std::string line;

	int i = 0;
	char * end = nullptr;
	while (std::getline (file, line, ';')) {
	    unsigned long value = std::strtoll (line.c_str (), &end, 10);
	    packet.globalMetrics [i].value = value;
	}
	
	file.close ();
    }
      
    void RaplReader::waitNfsModification () {
	this-> _nfs.readTimer.sleep (this-> _nfs.pollSpeed);

	while (true) {
	    struct stat st;
	    auto fd = fopen (this-> _nfs.valuePath.c_str (), "r"); // we have to open it to make nfs file is up to date....
	    if (fd == nullptr) {
		utils::Logger::globalInstance ().error ("Value file disappeard in nfs directory.");
		this-> waitNfsAvailability ();
		continue;
	    }

	    fclose (fd);
	    
	    if (stat (this-> _nfs.valuePath.c_str (), &st) == -1) {
		throw std::runtime_error ("");
	    }

	    if (st.st_mtim.tv_sec != this-> _nfs.lastPollSec || st.st_mtim.tv_nsec != this-> _nfs.lastPollnSec) {
		this-> _nfs.lastPollSec = st.st_mtim.tv_sec;
		this-> _nfs.lastPollnSec = st.st_mtim.tv_nsec;

		auto freq = fopen (this-> _nfs.freqPath.c_str (), "r");
		if (freq != nullptr) {
		    auto ig = fscanf (freq, "%lf", &this-> _nfs.pollSpeed);
		    fclose (freq);
		}
		
		break;
	    } else {
		this-> _nfs.readTimer.sleep (0.1);
		utils::Logger::globalInstance ().debug ("Host frequency recalibration, wait 0.1 second.");
	    }
	}

	utils::Logger::globalInstance ().debug ("Host frequency of writing is : ", 1.0 / this-> _nfs.pollSpeed);
	this-> _nfs.readTimer.reset ();
    }    

    /**
     * ================================================================================
     * ================================================================================
     * ===========================        DISPOSING         ===========================
     * ================================================================================
     * ================================================================================
     */
    
    void RaplReader::dispose () {
	if (this-> _mode == RaplReaderMode::BARE_METAL) {
	    for (auto & it : this-> _bare.fds) {
		close (it);
	    }

	    this-> _bare.fds.clear ();
	    this-> _bare.packageUnits.clear ();
	    this-> _bare.totalCores = 0;
	    this-> _bare.totalPackages = 0;
	    this-> _bare.cpuModel = -1;
	    this-> _bare.raplAvail = {};
	    this-> _bare.packageMap.clear ();
	    this-> _bare.tempFile = "";
	    this-> _bare.freqFiles.clear ();
	} else if (this-> _mode == RaplReaderMode::TCP) {
	} else {
	    auto saveSpeed = this-> _nfs.pollSpeed;
	    this-> dismountNfs ();
	    this-> _nfs = {};
	    this-> _nfs.pollSpeed = saveSpeed; // dismount can mean that we will remount in the future so we need to keep track of the sensor speed
	}	
    }
    
    RaplReader::~RaplReader () {
	this-> dispose ();
    }
}
