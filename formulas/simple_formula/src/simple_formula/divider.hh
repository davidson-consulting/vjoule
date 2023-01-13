#pragma once

#include <map>
#include <string>
#include <common/net/_.hh>
#include <common/formula/divider.hh>

namespace simple_formula {
    
    /**
     * The divider class transform packets into cgroup consumption
     */
    class Divider : public common::formula::Divider {
    private:

	short _llc_id;

	short _clk_id;
	
    public :

	Divider (common::formula::RaplType type);

	/**
	 * Configure the divider from the header
	 */
	void configure (const common::net::Header & head) override;
	
	/**
	 * Divide the consumption from packet information into cgroup consumption
	 * @returns: the consumption of each packet
	 */
	std::map <std::string, common::formula::PackageValue> divideConsumption (const common::net::Packet & packet) override;

	/**
	 * @returns: the list of necessary system metrics sent by the sensor for the divider to work
	 */
	std::vector<std::string> getSystemMetrics () const override;

	/**
	 * @returns: the list of necessary cgroup metrics sent by the sensor for the divider to work
	 */
	std::vector<std::string> getCgroupMetrics () const override;

		
    };

    
}
