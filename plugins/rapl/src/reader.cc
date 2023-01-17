#include "reader.hh"
#include "utils.hh"
#include <common/utils/log.hh>
#include <common/utils/files.hh>

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

namespace fs = std::filesystem;

namespace rapl {

    RaplReader::RaplReader () {}        

    bool RaplReader::configure () {	
	return this-> openMsrFiles ();
    }
    
    bool RaplReader::openMsrFiles () {
	LOG_INFO ("RaplReader : try to configure in bare metal mode.");
	this-> _cpuModel = detect_cpu ();
	this-> _packageMap = detect_packages (this-> _totalPackages, this-> _totalCores);	
	this-> _raplAvail = detect_avail (0, this-> _cpuModel);
	
	try {
	    for (auto & it: this-> _packageMap) {
		this-> _fds.push_back (open_msr (it));

		auto packageUnit = read_package_unit (this-> _fds.back (), this-> _raplAvail);
		this-> _packageUnits.push_back (packageUnit);

		LOG_INFO ("Power units[", it, "] = ", packageUnit.powerUnits, "W");
		LOG_INFO ("CPU energy units[", it, "] = ",  packageUnit.cpuEnergyUnits, "J");
		LOG_INFO ("DRAM energy units[", it, "] = ", packageUnit.dramEnergyUnits, "J");
		LOG_INFO ("Time units[", it, "] = ", packageUnit.timeUnits, "s");
		this-> _lastValues.push_back (read_package_values (this-> _fds.back (), this-> _raplAvail, packageUnit));
	    }

	    LOG_INFO ("RaplReader : bare metal mode is available.");

	    if (this-> _raplAvail.dram) {
		LOG_INFO ("Rapl DRAM available.");
	    } else LOG_WARN ("Rapl DRAM not available.");

	    if (this-> _raplAvail.pp1) {
		LOG_INFO ("Rapl PP1 (integrated GPU) available.");
	    } else LOG_WARN ("Rapl PP1 (integrated GPU) not available.");
	    
	    return true;
	} catch (...) {
	    this-> dispose (); // disposing because some elements where opened even if they failed to provide data

	    LOG_ERROR ("RaplReader : failed to configure.");
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

    
    void RaplReader::poll () {
	this-> _energy_pp0 = 0;
	this-> _energy_pp1 = 0;
	this-> _energy_dram = 0;
	this-> _energy_pkg = 0;
	this-> _energy_psys = 0;
	
	for (size_t i = 0 ; i < this-> _fds.size () ; i++) {
	    auto values = read_package_values (this-> _fds[i], this-> _raplAvail, this-> _packageUnits[i]);
	    this-> _energy_pp0 += (values.pp0 - this-> _lastValues[i].pp0);
	    this-> _energy_pp1 += (values.pp1 - this-> _lastValues[i].pp1);
	    this-> _energy_dram += (values.dram - this-> _lastValues[i].dram);
	    this-> _energy_pkg += (values.package - this-> _lastValues[i].package);
	    this-> _energy_psys += (values.psys - this-> _lastValues[i].psys);
	    
	    this-> _lastValues [i] = values;
	}       
    }


    float RaplReader::getCpuEnergy () const {
	return this-> _energy_pkg;
    }

    float RaplReader::getRamEnergy () const {
	return this-> _energy_dram;
    }

    float RaplReader::getGpuEnergy () const {
	return this-> _energy_pp1;
    }
    

    /**
     * ================================================================================
     * ================================================================================
     * ===========================        DISPOSING         ===========================
     * ================================================================================
     * ================================================================================
     */
    
    void RaplReader::dispose () {
	for (auto & it : this-> _fds) {
	    close (it);
	}

	this-> _fds.clear ();
	this-> _packageUnits.clear ();
	this-> _totalCores = 0;
	this-> _totalPackages = 0;
	this-> _cpuModel = -1;
	this-> _raplAvail = {};
	this-> _packageMap.clear ();
	this-> _tempFile = "";
	this-> _freqFiles.clear ();
    }
    
    RaplReader::~RaplReader () {
	this-> dispose ();
    }
}
