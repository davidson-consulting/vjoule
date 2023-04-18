#include "utils.hh"
#include <common/utils/log.hh>
#include <stdexcept>
#include <common/utils/files.hh>
#include <string>

#include <sys/syscall.h>
#include <linux/perf_event.h>
#include <sys/ioctl.h>

using namespace common;

namespace rapl {


	/**
	 * ================================================================================
	 * ================================================================================
	 * ============================          MSR          =============================
	 * ================================================================================
	 * ================================================================================
	 */

    int open_msr(int core) {
		char msr_filename[BUFSIZ];
		int fd;

		sprintf(msr_filename, "/dev/cpu/%d/msr", core);
		fd = open(msr_filename, O_RDONLY);
		if ( fd < 0 ) {
			if ( errno == ENXIO ) {
				LOG_ERROR ("rdmsr: No CPU ", core);
				throw std::runtime_error ("");
			} else if ( errno == EIO ) {
				LOG_ERROR ("rdmsr: CPU ", core, " doesn't support MSRs");
				throw std::runtime_error ("");
			} else {
				LOG_ERROR ("rdmsr:open, Failed to open msr ", msr_filename);
				throw std::runtime_error ("");
			}
		}

		return fd;
    }

    uint64_t read_msr_no_cache (int fd, int which) {

		uint64_t data;

		if ( pread(fd, &data, sizeof data, which) != sizeof data ) {
			LOG_ERROR ("rdmsr:Failed to read msr data");
			throw std::runtime_error ("");
		}
	
		return data;
    }

    
    uint64_t read_msr(int fd, int which, uint64_t & old) {

		uint64_t data;

		if ( pread(fd, &data, sizeof data, which) != sizeof data ) {
			LOG_ERROR ("rdmsr:Failed to read msr data");
			throw std::runtime_error ("");
		}

		if (old != 0 && old > data) {
			auto n_data = (4294967295 - old) + data;
			old = data;
			return n_data;
		} else if (old == 0) {
			old = data;
			return 0;
		} else {
			auto n_data = data - old;
			old = data;
			return n_data;
		}
    }


    int detect_cpu(void) {

		FILE *fff;

		int family,model=-1;
		char buffer[BUFSIZ],*result;
		char vendor[BUFSIZ];

		fff=fopen("/proc/cpuinfo","r");
		if (fff==NULL) return -1;

		while(1) {
			result=fgets(buffer,BUFSIZ,fff);
			if (result==NULL) break;

			if (!strncmp(result,"vendor_id",8)) {
				sscanf(result,"%*s%*s%s",vendor);

				if (strncmp(vendor,"GenuineIntel",12)) {
					LOG_WARN (vendor, " not an intel chip");
				}
			}

			if (!strncmp(result,"cpu family",10)) {
				sscanf(result,"%*s%*s%*s%d",&family);
				if (family!=6) {
					LOG_ERROR ("Wrong cpu family : ", family);
					throw std::runtime_error ("");
				}
			}

			if (!strncmp(result,"model",5)) {
				sscanf(result,"%*s%*s%d",&model);
			}

		}

		fclose(fff);

		switch(model) {
		case CPU_SANDYBRIDGE:
			LOG_INFO ("CPU arch : Sandybridge");
			break;
		case CPU_SANDYBRIDGE_EP:
			LOG_INFO ("CPU arch : Sandybridge-EP");
			break;
		case CPU_IVYBRIDGE:
			LOG_INFO ("CPU arch : Ivybridge");
			break;
		case CPU_IVYBRIDGE_EP:
			LOG_INFO ("CPU arch : Ivybridge-EP");
			break;
		case CPU_HASWELL:
		case CPU_HASWELL_ULT:
		case CPU_HASWELL_GT3E:
			LOG_INFO ("CPU arch : Haswell");
			break;
		case CPU_HASWELL_EP:
			LOG_INFO ("CPU arch : Haswell-EP");
			break;
		case CPU_BROADWELL:
		case CPU_BROADWELL_GT3E:
			LOG_INFO ("CPU arch : Broadwell");
			break;
		case CPU_BROADWELL_EP:
			LOG_INFO ("CPU arch : Broadwell-EP");
			break;
		case CPU_SKYLAKE:
		case CPU_SKYLAKE_HS:
			LOG_INFO ("CPU arch : Skylake");
			break;
		case CPU_SKYLAKE_X:
			LOG_INFO ("CPU arch : Skylake-X");
			break;
		case CPU_KABYLAKE:
		case CPU_KABYLAKE_MOBILE:
			LOG_INFO ("CPU arch : Kaby Lake");
			break;
		case CPU_KNIGHTS_LANDING:
			LOG_INFO ("CPU arch : Knight's Landing");
			break;
		case CPU_KNIGHTS_MILL:
			LOG_INFO ("CPU arch : Knight's Mill");
			break;
		case CPU_TIGERLAKE :
			LOG_INFO ("CPU arch : Tiger Lake");
			break;
		case CPU_ATOM_GOLDMONT:
		case CPU_ATOM_GEMINI_LAKE:
		case CPU_ATOM_DENVERTON:
			LOG_INFO ("CPU arch : Atom");
			break;
		default:
			LOG_INFO ("CPU arch : unsupported");
			model=-1;
			break;
		}

		return model;
    }

    std::vector<int> detect_packages(int& total_packages, int& total_cores) {
		total_packages = 0;
		total_cores = 0;
	
		std::vector<int> package_map;
		char filename[BUFSIZ];
		int package;
		int i;

		for(i = 0 ; i < MAX_CPUS ; i++) {
			sprintf(filename,"/sys/devices/system/cpu/cpu%d/topology/physical_package_id", i);
			auto fff = fopen(filename,"r");
			if (fff == NULL) {
				break;
			} else {
				if (fscanf (fff, "%d", &package) == 1) {
					if (package_map.size () <= package) {
						package_map.resize (package + 1, -1);
						total_packages++;
						package_map [package] = i;
					}
				}
				fclose(fff);
			}
		}

		total_cores = i;
		LOG_INFO ("Detected ", total_cores, " cores in ", total_packages, " packages");
		return package_map;
    }

    EventAvail detect_avail (int core, int cpu_model) {
		EventAvail avail = {false, false, false, false, false};
		avail.dram = utils::file_exists (POWER_ENERGY_RAM);
		avail.pp0 = utils::file_exists (POWER_ENERGY_PP0);
		avail.pp1 = utils::file_exists (POWER_ENERGY_PP1);
		avail.psys = utils::file_exists (POWER_ENERGY_PSYS);

		avail.different_units = (cpu_model == CPU_HASWELL_EP ||
								 cpu_model == CPU_BROADWELL_EP ||
								 cpu_model == CPU_SKYLAKE_X ||
								 cpu_model == CPU_KNIGHTS_LANDING ||
								 cpu_model == CPU_KNIGHTS_MILL);
		return avail;
    }

    PackageUnits read_package_unit (int fd, EventAvail avail) {
		auto result = read_msr_no_cache (fd, MSR_RAPL_POWER_UNIT);

		PackageUnits pack;
		pack.powerUnits = pow(0.5,(double)(result&0xf));
		pack.cpuEnergyUnits = pow(0.5,(double)((result>>8)&0x1f));
		pack.timeUnits = pow(0.5,(double)((result>>16)&0xf));

		if (avail.dram) {
			if (avail.different_units) {
				pack.dramEnergyUnits = pow (0.5, (double)16);
			} else {
				pack.dramEnergyUnits = pack.cpuEnergyUnits;
			}
		}
	
		result = read_msr_no_cache (fd, MSR_PKG_POWER_INFO);
		pack.thermalSpecPower = pack.powerUnits * (double)(result & 0x7fff);
		pack.minimumPower = pack.powerUnits * (double)((result >> 16) & 0x7fff);
		pack.maximumPower = pack.powerUnits * (double) ((result >> 32) & 0x7fff);
		pack.timeWindow = pack.timeUnits * (double) ((result >> 48) & 0x7fff);

		return pack;
    }


    PackageContent read_package_values (int fd, EventAvail avail, PackageUnits unit, PackageCache & cache) {
		PackageContent pack;
		auto result = read_msr (fd, MSR_PKG_ENERGY_STATUS, cache.package);
		pack.package = ((double)result) * unit.cpuEnergyUnits;

		if (avail.pp0) {
			result = read_msr (fd, MSR_PP0_ENERGY_STATUS, cache.pp0);
			pack.pp0 = ((double)result) * unit.cpuEnergyUnits;
		}

		if (avail.pp1) {
			result = read_msr (fd, MSR_PP1_ENERGY_STATUS, cache.pp1);
			pack.pp1 = ((double)result) * unit.cpuEnergyUnits;
		}

		if (avail.dram) {
			result = read_msr (fd, MSR_DRAM_ENERGY_STATUS, cache.dram);
			pack.dram = ((double)result) * unit.dramEnergyUnits;
		}
	
		if (avail.psys) {
			result = read_msr (fd, MSR_PLATFORM_ENERGY_STATUS, cache.psys);
			pack.psys = ((double)result) * unit.cpuEnergyUnits;
		}


		return pack;
    }


	/**
	 * ================================================================================
	 * ================================================================================
	 * ========================          PERF_EVENT          ==========================
	 * ================================================================================
	 * ================================================================================
	 */

	char rapl_domain_names[NUM_RAPL_DOMAINS][30]= {
		"energy-pkg",
		"energy-cores",
		"energy-gpu",
		"energy-ram",
		"energy-psys",
	};

	static int perf_event_open(struct perf_event_attr *hw_event_uptr,
							   pid_t pid, int cpu, int group_fd, unsigned long flags) {

        return syscall(__NR_perf_event_open,hw_event_uptr, pid, cpu,
					   group_fd, flags);
	}

	int perf_event_check_paranoid (void) {
		int paranoid_value;
		FILE *fff;

		fff=fopen("/proc/sys/kernel/perf_event_paranoid","r");
		if (fff==NULL) {
			LOG_ERROR ("Error! could not open /proc/sys/kernel/perf_event_paranoid ", strerror(errno));

			/* We can't return a negative value as that implies no paranoia */
			return 500;
		}

		fscanf(fff,"%d",&paranoid_value);
		fclose(fff);

		return paranoid_value;
	}


	int perf_event_get_event_type () {
		FILE * fff=fopen("/sys/bus/event_source/devices/power/type","r");
		int type;

		if (fff == NULL) {
			LOG_ERROR ("\tNo perf_event rapl support found (requires Linux 3.14)\n")
			return -1;
		}

		fscanf(fff,"%d", &type);
		fclose(fff);

		return type;
	}

	EventAvail perf_event_open_packages (PackageUnits & units, std::vector <int> & packageMap, std::vector <int> & fds) {
		fds.resize (packageMap.size () * NUM_RAPL_DOMAINS);
		int type = perf_event_get_event_type ();

		FILE * fff = NULL;
		char filename [255];
		int config [NUM_RAPL_DOMAINS];
		struct perf_event_attr attr;
		double scale;

		EventAvail avail = {false, false, false, false, false};
		for (uint64_t i = 0 ; i < NUM_RAPL_DOMAINS ; i++) {
			config [i] = 0;
			sprintf(filename,"/sys/bus/event_source/devices/power/events/%s", rapl_domain_names[i]);
			fff = fopen(filename,"r");

			if (fff != NULL) {
				fscanf(fff,"event=%x", &config[i]);
				fclose(fff);
			} else continue;

			sprintf(filename,"/sys/bus/event_source/devices/power/events/%s.scale", rapl_domain_names[i]);
			fff = fopen(filename,"r");

			if (fff != NULL) {
				fscanf(fff,"%lf", &scale);
				fclose(fff);
			}

			switch (i) {
			case 0 :
				units.cpuEnergyUnits = scale;
				break;
			case 1:
				avail.pp0 = true;
				break;
			case 2:
				avail.pp1 = true;
				break;
			case 3:
				avail.dram = true;
				units.dramEnergyUnits = scale;
				break;
			default:
				avail.psys = true;
			}

		}

		for (uint64_t i = 0; i < packageMap.size () ; i++) {
			for (uint64_t j = 0 ; j < NUM_RAPL_DOMAINS ; j++) {
				fds [i * NUM_RAPL_DOMAINS + j] = -1;
				memset (&attr, 0, sizeof (attr));
				attr.type = type;
				attr.config = config [j];

				if (config [j] == 0) continue;

				fds [i * NUM_RAPL_DOMAINS + j] = perf_event_open (&attr, -1, packageMap [i], -1, 0);
				if (fds [i * NUM_RAPL_DOMAINS + j] < 0) {
					if (errno == EACCES) {
						int paranoid_value = perf_event_check_paranoid ();
						if (paranoid_value > 0) {
							LOG_ERROR ("/proc/sys/kernel/perf_event_paranoid is ", paranoid_value, " but it must be 0 or lower\n");
							throw std::runtime_error ("");
						}
					} else {
						LOG_ERROR ("opening core ", i, " ", packageMap [i], " config ", config [j], " ", strerror (errno));
						throw std::runtime_error ("");
					}
				}

			}
		}

		return avail;
	}

	PackageContent perf_event_read_package_values (uint64_t pMap, EventAvail avail, PackageUnits units, std::vector <int> & fds) {
		long long value;

		PackageContent pack = {};
		read (fds [pMap * NUM_RAPL_DOMAINS], &value, 8);
		pack.package = ((double) value) * units.cpuEnergyUnits;

		ioctl (fds[pMap * NUM_RAPL_DOMAINS], PERF_EVENT_IOC_RESET);

		if (avail.pp0) {
			read (fds [pMap * NUM_RAPL_DOMAINS + 1], &value, 8);
			pack.pp0 = ((double) value) * units.cpuEnergyUnits;
			ioctl (fds[pMap * NUM_RAPL_DOMAINS + 1], PERF_EVENT_IOC_RESET);
		}

		if (avail.pp1) {
			read (fds [pMap * NUM_RAPL_DOMAINS + 2], &value, 8);
			pack.pp1 = ((double) value) * units.cpuEnergyUnits;
			ioctl (fds[pMap * NUM_RAPL_DOMAINS + 2], PERF_EVENT_IOC_RESET);
		}

		if (avail.dram) {
			read (fds [pMap * NUM_RAPL_DOMAINS + 3], &value, 8);
			pack.dram = ((double) value) * units.dramEnergyUnits;
			ioctl (fds[pMap * NUM_RAPL_DOMAINS + 3], PERF_EVENT_IOC_RESET);
		}
		
		if (avail.psys) {
			read (fds [pMap * NUM_RAPL_DOMAINS + 4], &value, 8);
			pack.psys = ((double) value) * units.cpuEnergyUnits;
			ioctl (fds[pMap * NUM_RAPL_DOMAINS + 4], PERF_EVENT_IOC_RESET);
		}

		return pack;
	}


	/**
	 * ================================================================================
	 * ================================================================================
	 * ==========================          POWERCAP          ==========================
	 * ================================================================================
	 * ================================================================================
	 */




}
