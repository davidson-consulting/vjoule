#include <simple_formula/divider.hh>
#include <common/utils/log.hh>

using namespace common;

namespace simple_formula {

    Divider::Divider (formula::RaplType type) : formula::Divider (type) {}    


    void Divider::configure (const common::net::Header & head) {
	/**
	 * ##################################################################################
	 * ##################################################################################
	 * ########################### TODO CHANGE HERE IN NEW FORMULA ######################
	 * ##################################################################################
	 * ##################################################################################
	 */

	auto in = common::net::reverse (head);
	auto llcFnd = in.metricIds.find ("LLC_MISSES");
	if (llcFnd != in.metricIds.end ()) { // store the llc_id, so we don't look for it at each iteration
	    this-> _llc_id = llcFnd-> second;
	}

	auto clkFnd = in.metricIds.find ("PERF_COUNT_HW_CPU_CYCLES");
	if (clkFnd != in.metricIds.end ()) { // same for clk id
	    this-> _clk_id = clkFnd-> second;
	}

	utils::Logger::globalInstance ().debug ("LLC : ", this-> _llc_id, " CPU_CYCLES ", this-> _clk_id);

	/**
	 * ##################################################################################
	 * ##################################################################################
	 * ############################ TODO END OF CHANGE HERE #############################
	 * ##################################################################################
	 * ##################################################################################
	 */
    }
    
    std::map <std::string, formula::PackageValue> Divider::divideConsumption (const common::net::Packet & packet) {
	std::map <std::string, formula::PackageValue> result;
	
	/**
	 * ##################################################################################
	 * ##################################################################################
	 * ########################### TODO CHANGE HERE IN NEW FORMULA ######################
	 * ##################################################################################
	 * ##################################################################################
	 */
	
	unsigned long sum_cpu = 0;
	unsigned long sum_ram = 0;
	for (auto & it : packet.cgroupPackets) { // we compute the sum of consumption
	    for (auto & j : it.metrics) {
		if (j.id == this-> _llc_id) sum_ram += j.value;
		else if (j.id == this-> _clk_id) sum_cpu += j.value;
	    }
	}

	if (packet.energy_pkg > 0) {
	    for (auto &it : packet.cgroupPackets) { // and the proportion for each cgroups
		formula::PackageValue value{};
		for (auto & j : it.metrics) {
		    if (j.id == this-> _llc_id) {
			if (sum_ram != 0) {
			    value.dram = ((double)j.value / (double) (sum_ram)) * packet.energy_dram;
			}
		    } else if (j.id == this-> _clk_id) {
			if (sum_cpu != 0) {
			    value.package = ((double)j.value / (double) (sum_cpu)) * packet.energy_pkg;
			}
		    }
		}
		result.emplace (it.id, value);
	    }
	}

	return result;

	/**
	 * ##################################################################################
	 * ##################################################################################
	 * ############################ TODO END OF CHANGE HERE #############################
	 * ##################################################################################
	 * ##################################################################################
	 */
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
