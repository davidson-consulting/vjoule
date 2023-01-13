#include <divider.hh>
#include <sys/time.h>
#include <common/utils/_.hh>

using namespace common;

namespace dumping_formula {

    Divider::Divider (formula::RaplType type) :
	formula::Divider (type)
    {
	std::string endName;
	switch (type) {
	case formula::RaplType::MAX: 
	    endName = "_max.csv";
	    break;
	case formula::RaplType::MIN:
	    endName = "_min.csv";
	    break;
	case formula::RaplType::MOY:
	    endName = "_moy.csv";
	    break;
	default :
	    endName = ".csv";
	    break;
	}

	this-> _rapl = std::ofstream (std::string (VJOULE_DIR) + "dumping_formula/rapl" + endName, std::ios::out);
	this-> _cgroups = std::ofstream (std::string (VJOULE_DIR) + "dumping_formula/cgroups.csv", std::ios::out);
    }    


    void Divider::configure (const common::net::Header & head) {	
	this-> _cgroups << "TIMESTAMP ; ID ; ";
	int i = 0;
	for (auto & it : head.metricNames) {
	    if (i != 0) this-> _cgroups << ";" ;
	    i += 1;
	    this-> _cgroups << it.second;
	}

	this-> _cache.resize (head.metricNames.size ());
	
	this-> _cgroups << std::endl;	
	this-> _rapl << "TIMESTAMP ; PACKAGE ; DRAM ; PP0 ; PP1 ; PSYS ; CORE_TEMP";
	for (int i = 0 ; i < head.nbCores ; i++) {
	    this-> _rapl << " ; " << "CORE_HZ_" << i;
	}
	this-> _rapl << "\n";
    }
    
    std::map <std::string, formula::PackageValue> Divider::divideConsumption (const common::net::Packet & packet) {
	struct timeval start;
	gettimeofday(&start, NULL);
	
	this-> _rapl << start.tv_sec << "." << start.tv_usec << ";" << packet.energy_pkg << ";" << packet.energy_dram << ";" << packet.energy_pp0 << ";" << packet.energy_pp1 << ";" << packet.energy_psys << ";" << packet.core_temp;
	for (int i = 0 ; i < packet.cpuFreq.size () ; i++) {
	    this-> _rapl << " ; " << packet.cpuFreq[i];
	}
	this-> _rapl << std::endl;

	this-> _cgroups <<  start.tv_sec << "." << start.tv_usec << ";#SYSTEM;";
	
	std::fill(this-> _cache.begin(), this-> _cache.end(), 0);
	for (auto & metric : packet.globalMetrics) {
	    this-> _cache [metric.id] = metric.value;
	}

	int i = 0;
	for (auto & it : this-> _cache) {
	    if (i != 0) this-> _cgroups << ";";
	    i += 1;
	    this-> _cgroups << it;
	}	
	this-> _cgroups << std::endl;

	
	for (auto & cgroup : packet.cgroupPackets) {
	    this-> _cgroups << start.tv_sec << "." << start.tv_usec << ";" << this-> simplifyName (cgroup.id) << ";";

	    std::fill(this-> _cache.begin(), this-> _cache.end(), 0);
	    for (auto & metric : cgroup.metrics) {
		this-> _cache [metric.id] = metric.value;
	    }
	    
	    int i = 0;
	    for (auto & it : this-> _cache) {
		if (i != 0) this-> _cgroups << ";";
		i += 1;
		this-> _cgroups << it;
	    }
	    this-> _cgroups << std::endl;
	}
	
	return {};
    }

  std::string Divider::simplifyName (const std::string & name) const {
    if (name.find ("machine-qemu") != std::string::npos) {
      auto pos = name.find ("machine-qemu");
      auto pos2 = name.rfind ("\\x2d") + 4;
      auto pos3 = name.rfind (".scope");

	    
      auto res = name.substr (0, pos) + name.substr (pos2, pos3 - pos2);
      return res;
    }

    return name;
  }

  
    /**
     * @returns: the list of necessary system metrics sent by the sensor for the divider to work
     */
    std::vector<std::string> Divider::getSystemMetrics () const {
	return {};
    }

    /**
     * @returns: the list of necessary cgroup metrics sent by the sensor for the divider to work
     */
    std::vector<std::string> Divider::getCgroupMetrics () const {
	return {"PERF_COUNT_HW_CPU_CYCLES", "LLC_MISSES"};
    }
    
    

}
