#pragma once

#ifndef __PROJECT__
#define __PROJECT__ "RAPL"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include <sys/syscall.h>
#include <linux/perf_event.h>
#include <vector>

namespace rapl {

/**
	 * ================================================================================
	 * ================================================================================
	 * ============================          MSR          =============================
	 * ================================================================================
	 * ================================================================================
	 */


#define MSR_RAPL_POWER_UNIT		0x606

    /*
     * Platform specific RAPL Domains.
     * Note that PP1 RAPL Domain is supported on 062A only
     * And DRAM RAPL Domain is supported on 062D only
     */
    /* Package RAPL Domain */
#define MSR_PKG_RAPL_POWER_LIMIT	0x610
#define MSR_PKG_ENERGY_STATUS		0x611
#define MSR_PKG_PERF_STATUS		0x613
#define MSR_PKG_POWER_INFO		0x614

    /* PP0 RAPL Domain */
#define MSR_PP0_POWER_LIMIT		0x638
#define MSR_PP0_ENERGY_STATUS		0x639
#define MSR_PP0_POLICY			0x63A
#define MSR_PP0_PERF_STATUS		0x63B

    /* PP1 RAPL Domain, may reflect to uncore devices */
#define MSR_PP1_POWER_LIMIT		0x640
#define MSR_PP1_ENERGY_STATUS		0x641
#define MSR_PP1_POLICY			0x642

    /* DRAM RAPL Domain */
#define MSR_DRAM_POWER_LIMIT		0x618
#define MSR_DRAM_ENERGY_STATUS		0x619
#define MSR_DRAM_PERF_STATUS		0x61B
#define MSR_DRAM_POWER_INFO		0x61C

    /* PSYS RAPL Domain */
#define MSR_PLATFORM_ENERGY_STATUS	0x64d

    /* RAPL UNIT BITMASK */
#define POWER_UNIT_OFFSET	0
#define POWER_UNIT_MASK		0x0F

#define ENERGY_UNIT_OFFSET	0x08
#define ENERGY_UNIT_MASK	0x1F00

#define TIME_UNIT_OFFSET	0x10
#define TIME_UNIT_MASK		0xF000

    /**
     * Open the file msr for core 'core'
     * @returns: the file descriptor
     * @throws: std::runtime_error
     */
    int open_msr(int core);

    /**
     * Read the value of the msr file descriptor
     */
    uint64_t read_msr(int fd, int which, uint64_t & old);

    uint64_t read_msr_no_cache(int fd, int which);


#define CPU_SANDYBRIDGE		42
#define CPU_SANDYBRIDGE_EP	45
#define CPU_IVYBRIDGE		58
#define CPU_IVYBRIDGE_EP	62
#define CPU_HASWELL		60
#define CPU_HASWELL_ULT		69
#define CPU_HASWELL_GT3E	70
#define CPU_HASWELL_EP		63
#define CPU_BROADWELL		61
#define CPU_BROADWELL_GT3E	71
#define CPU_BROADWELL_EP	79
#define CPU_BROADWELL_DE	86
#define CPU_SKYLAKE		78
#define CPU_SKYLAKE_HS		94
#define CPU_SKYLAKE_X		85
#define CPU_KNIGHTS_LANDING	87
#define CPU_KNIGHTS_MILL	133
#define CPU_KABYLAKE_MOBILE	142
#define CPU_TIGERLAKE           140
#define CPU_KABYLAKE		158
#define CPU_ATOM_SILVERMONT	55
#define CPU_ATOM_AIRMONT	76
#define CPU_ATOM_MERRIFIELD	74
#define CPU_ATOM_MOOREFIELD	90
#define CPU_ATOM_GOLDMONT	92
#define CPU_ATOM_GEMINI_LAKE	122
#define CPU_ATOM_DENVERTON	95


#define POWER_ENERGY_PP0 "/sys/bus/event_source/devices/power/events/energy-cores"
#define POWER_ENERGY_PP1 "/sys/bus/event_source/devices/power/events/energy-gpu"
#define POWER_ENERGY_PKG "/sys/bus/event_source/devices/power/events/energy-pkg"
#define POWER_ENERGY_RAM "/sys/bus/event_source/devices/power/events/energy-ram"
#define POWER_ENERGY_RAM_SCALE "/sys/bus/event_source/devices/power/events/energy-ram.scale"
#define POWER_ENERGY_PSYS "/sys/bus/event_source/devices/power/events/energy-psys"
        
    /**
     * @returns: the model of cpu
     */
    int detect_cpu();

#define MAX_CPUS	1024
#define MAX_PACKAGES	16

    /**
     * @returns:
     *   - totalPackages: the number of available packages
     *   - totalCores: the number of cores
     */
    std::vector<int> detect_packages(int& totalPackages, int& totalCores);


    struct EventAvail {
        bool dram;
        bool pp0;
        bool pp1;
        bool psys;
        bool different_units;
    };

    struct PackageCache {
        uint64_t package;
        uint64_t pp0;
        uint64_t pp1;
        uint64_t dram;
        uint64_t psys;
    };
    
    struct PackageUnits {
        // The unit of measurement of power
        double powerUnits;

        // The unit of measurement of cpu energy
        double cpuEnergyUnits;

        // The unit of measurement of dram energy
        double dramEnergyUnits;
	
        // The unit of measurement for time
        double timeUnits;

        // The thermal information
        double thermalSpecPower;

        // The minimal power of package
        double minimumPower;

        // The maximal power of the package
        double maximumPower;

        // The time window of a package
        double timeWindow;
    };

    /**
     * @returns: the availability of events for the given core
     */
    EventAvail detect_avail (int core, int cpu_model);

    /**
     * Read the units of an open package
     */
    PackageUnits read_package_unit (int fd, EventAvail avail);


    struct PackageContent {
        double package;
        double pp0;
        double pp1;
        double dram;
        double psys;
    };

    /**
     * Read the content of a msr package
     */
    PackageContent read_package_values (int fd, EventAvail avail, PackageUnits units, PackageCache & cache);


	/**
	 * ================================================================================
	 * ================================================================================
	 * ========================          PERF_EVENT          ==========================
	 * ================================================================================
	 * ================================================================================
	 */

#define NUM_RAPL_DOMAINS	5
#define MAX_PACKAGES	16

	EventAvail perf_event_open_packages (PackageUnits & units, std::vector <int> & packageMap, std::vector <int> & fds);

    PackageContent perf_event_read_package_values (uint64_t pMap, EventAvail avail, PackageUnits units, std::vector <int> & fds);

	/**
	 * ================================================================================
	 * ================================================================================
	 * ==========================          POWERCAP          ==========================
	 * ================================================================================
	 * ================================================================================
	 */


}
