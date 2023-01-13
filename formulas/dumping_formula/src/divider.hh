#pragma once

#include <map>
#include <string>
#include <common/net/_.hh>
#include <common/formula/divider.hh>
#include <fstream>

namespace dumping_formula {
    
    /**
     * The divider class does not divide but writes the packet results to files
     */
    class Divider : public common::formula::Divider {
    private:

	short _llc_id;

	short _clk_id;

	std::ofstream _rapl;

	std::ofstream _cgroups;

	std::vector <unsigned long> _cache;
	
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


    private :

      std::string simplifyName (const std::string & name) const;
      
    };

    
}
