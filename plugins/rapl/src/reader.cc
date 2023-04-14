#include "reader.hh"
#include "utils.hh"
#include <common/utils/log.hh>
#include <common/utils/files.hh>

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


namespace rapl {

    RaplReader::RaplReader () {}        

    bool RaplReader::configure () {
        if (this-> openMsrFiles ()) {
            return true;
        } else if (this-> openPerfEvent ()) {
            return true;
        } else {
            return this-> openPowercap ();
        }
    }
    
    bool RaplReader::openMsrFiles () {
        try {
            this-> _cpuModel = detect_cpu ();
            this-> _packageMap = detect_packages (this-> _totalPackages, this-> _totalCores);
            this-> _raplAvail = detect_avail (0, this-> _cpuModel);

            for (auto & it: this-> _packageMap) {
                this-> _fds.push_back (open_msr (it));

                auto packageUnit = read_package_unit (this-> _fds.back (), this-> _raplAvail);
                this-> _packageUnits.push_back (packageUnit);

                LOG_INFO ("Power units[", it, "] = ", packageUnit.powerUnits, "W");
                LOG_INFO ("CPU energy units[", it, "] = ",  packageUnit.cpuEnergyUnits, "J");
                LOG_INFO ("DRAM energy units[", it, "] = ", packageUnit.dramEnergyUnits, "J");
                LOG_INFO ("Time units[", it, "] = ", packageUnit.timeUnits, "s");

                this-> _cache.push_back (PackageCache ());
                read_package_values (this-> _fds.back (), this-> _raplAvail, packageUnit, this-> _cache.back ());
            }

            LOG_INFO ("RaplReader : MSR is available.");

            if (this-> _raplAvail.dram) {
                LOG_INFO ("Rapl DRAM available.");
            } else LOG_WARN ("Rapl DRAM not available.");

            if (this-> _raplAvail.pp1) {
                LOG_INFO ("Rapl PP1 (integrated GPU) available.");
            } else LOG_WARN ("Rapl PP1 (integrated GPU) not available.");

            this-> _type = RaplReadType::MSR;

            LOG_INFO ("RaplReader : configured with MSR.");

            return true;
        } catch (...) {
            this-> dispose (); // disposing because some elements where opened even if they failed to provide data

            LOG_ERROR ("RaplReader : failed to configure with MSR.");
            return false;
        }
    }


    bool RaplReader::openPerfEvent () {
        try {
            this-> _packageMap = detect_packages (this-> _totalPackages, this-> _totalCores);
            this-> _packageUnits.push_back ({});
            this-> _raplAvail = perf_event_open_packages (this-> _packageUnits [0], this-> _packageMap, this-> _fds);

            LOG_INFO ("CPU energy units[", 0, "] = ",  this-> _packageUnits [0].cpuEnergyUnits, "J");
            LOG_INFO ("DRAM energy units[", 0, "] = ", this-> _packageUnits [0].dramEnergyUnits, "J");
            LOG_INFO ("RaplReader : Perf event is available.");

            if (this-> _raplAvail.dram) {
                LOG_INFO ("Rapl DRAM available.");
            } else LOG_WARN ("Rapl DRAM not available.");

            if (this-> _raplAvail.pp1) {
                LOG_INFO ("Rapl PP1 (integrated GPU) available.");
            } else LOG_WARN ("Rapl PP1 (integrated GPU) not available.");

            this-> _type = RaplReadType::PERF_EVENT;

            LOG_INFO ("RaplReader : configured with PERF_EVENT.");

            return true;
        } catch (...) {
            this-> dispose ();
            LOG_ERROR ("RaplReader : failed to configure with perf event.");
            return false;
        }
    }

    bool RaplReader::openPowercap () {
        return false;
    }

    /**
     * ================================================================================
     * ================================================================================
     * =========================        RESULT POLLING         ========================
     * ================================================================================
     * ================================================================================
     */

    
    void RaplReader::poll () {
        switch (this-> _type) {
        case RaplReadType::MSR:
            this-> pollMsr ();
            break;
        case RaplReadType::PERF_EVENT:
            this-> pollPerfEvent ();
            break;
        default:
            this-> pollPowercap ();
        }
    }

    void RaplReader::pollMsr () {
        this-> _energy_pp0 = 0;
        this-> _energy_pp1 = 0;
        this-> _energy_dram = 0;
        this-> _energy_pkg = 0;
        this-> _energy_psys = 0;
	
        for (size_t i = 0 ; i < this-> _fds.size () ; i++) {
            auto values = read_package_values (this-> _fds[i], this-> _raplAvail, this-> _packageUnits[i], this-> _cache[i]);
            this-> _energy_pp0 += values.pp0;
            this-> _energy_pp1 += values.pp1;
            this-> _energy_dram += values.dram;
            this-> _energy_pkg += values.package;
            this-> _energy_psys += values.psys;
        }
    }


    void RaplReader::pollPerfEvent () {
        this-> _energy_pp0 = 0;
        this-> _energy_pp1 = 0;
        this-> _energy_dram = 0;
        this-> _energy_pkg = 0;
        this-> _energy_psys = 0;

        for (size_t i = 0 ; i < this-> _packageMap.size () ; i++) {
            auto values = perf_event_read_package_values (i, this-> _raplAvail, this-> _packageUnits [0], this-> _fds);

            this-> _energy_pp0 += values.pp0;
            this-> _energy_pp1 += values.pp1;
            this-> _energy_dram += values.dram;
            this-> _energy_pkg += values.package;
            this-> _energy_psys += values.psys;
        }
    }

    void RaplReader::pollPowercap () {
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
    

    uint32_t RaplReader::getGpuNbDevices () const {
        if (this-> _gpuAvail) {
            return 1;
        } else return 0;
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
