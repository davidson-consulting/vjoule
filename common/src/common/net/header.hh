#pragma once

#include <map>
#include <vector>
#include <string>
#include <iostream>

namespace common::net {

    /**
     * Power limits for model computations
     */
    struct PowerLimits {
	float PMIN;
	float PMOY;
	float PMAX;
    };
    
    /**
     * The header is used to avoid sending metric names over and over at each frame
     */
    struct Header {
	
	// the number of cores
	int nbCores = 0;

	// True if the sensor is unable to find rapl
	bool simulation = false;

	// The power limit in case of simulation
	PowerLimits limits;
	
	// the consumption model of the machine (can be empty if not set in cfg file)
	std::vector <float> powerByCpus;
	
	// To metric id is associated a name
	std::map <short, std::string> metricNames;
    };

    /**
     * An inversed header is used to get metric ids from names 
     */
    struct InversedHeader {
    
	// To a name is associated a metric id
	std::map <std::string, short> metricIds;
    };

    /**
     * Transform raw datas into a readable header
     * @params: 
     *    - datas: the raw data to transform
     * @returns: a valid header
     */
    Header readHeaderFromBytes (const std::vector <char> & datas);


    /**
     * Transform a header into raw data
     * @params:
     *    - head: the header to transform
     * @returns: raw data
     */
    std::vector<char> writeHeaderToBytes (const Header& head);

    /**
     * Reverse a header
     * @params:
     *    - head: the header to reverse
     * @returns: a reversed header
     */
    InversedHeader reverse (const Header & head);

    /**
     * Reverse a header
     * @params:
     *    - head: the header to reverse
     * @returns: a reversed header
     */
    Header reverse (const InversedHeader& head);
}


std::ostream & operator<< (std::ostream & stream, const common::net::Header & head);
std::ostream & operator<< (std::ostream & stream, const common::net::InversedHeader & head);
bool operator!= (const common::net::Header & left, const common::net::Header & right);
