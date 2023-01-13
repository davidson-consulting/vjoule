#pragma once

#include <map>
#include <string>
#include <common/net/_.hh>
#include <common/formula/value.hh>

namespace common::formula {

    enum RaplType {
	DYN = 0, // dynamic rapl acquired by sensor
	MAX, // MAX rapl acquired by simulation on the formula
	MIN, // MIN rapl acquired by simulation on the formula
	MOY, // MOY rapl acquired by simulation on the formula
	END
    };    
    
    /**
     * The divider class transform packets into cgroup consumption
     */
    class Divider {
    protected :
	
	RaplType _raplType;
	
    public :

	Divider (RaplType type) : _raplType (type) {}

	/**
	 * Configure the divider from the header
	 */
	virtual void configure (const common::net::Header & head) = 0;
	
	/**
	 * Divide the consumption from packet information into cgroup consumption
	 * @returns: the consumption of each packet
	 */
	virtual std::map <std::string, common::formula::PackageValue> divideConsumption (const common::net::Packet & packet) = 0;

	/**
	 * @returns: the list of necessary system metrics sent by the sensor for the divider to work
	 */
	virtual std::vector<std::string> getSystemMetrics () const = 0;

	/**
	 * @returns: the list of necessary cgroup metrics sent by the sensor for the divider to work
	 */
	virtual std::vector<std::string> getCgroupMetrics () const = 0;
		
    };

    
}
