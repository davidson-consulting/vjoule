#pragma once

#include <vector>
#include <string>
#include <iostream>

namespace common::net {

    /**
     * Metric is a simple key-> value association
     * Where the key is an id of a packet header
     */
    struct Metric {
	// The id is described in packet header
	short id;

	// The value of the metric in the current packet
	long value;
    };

    /**
     * A cgroup packet contains the identifier and metric of a cgroup
     */
    struct CgroupPacket {    
	// The name of the cgroup
	std::string id;
    
	// The list of metric for the cgroup
	std::vector <Metric> metrics;
    };

  
    /**
     * A packet contains all the metrics (global and cgroups)
     */
    struct Packet {

	// energy of power plane 0
	float energy_pp0 = 0;

	// energy of power plane 1
	float energy_pp1 = 0;

	// energy pkg in joules
	float energy_pkg = 0;

	// energy ram in joules
	float energy_dram = 0;
	
	// energy psys in joules
	float energy_psys = 0;

	// cpu temperature
	unsigned int core_temp;

	// The cpu frequency of the cores
	std::vector <int> cpuFreq;
            
	// The list of global metrics
	std::vector <Metric> globalMetrics;

	// The list of cgroup packets
	std::vector<CgroupPacket> cgroupPackets;
    };

  
    /**
     * Transform raw datas into a readable packet
     * @params:
     *   - datas: the raw data to transform
     * @returns: a valid packet
     */
    Packet readPacketFromBytes (const std::vector<char> & datas);

    /**
     * Read metrics from raw datas
     * @params:
     *    - nbMetrics: the number of metrics to read
     *    - datas: the array of metrics
     * @returns:
     *    - metrics: the vector to populate 
     */
    void readMetricsFromBytes (std::vector<Metric> & metrics, int nbMetrics, const Metric* datas);

    /**
     * Read a cgroup from raw datas
     * @params:
     *    - datas: the data to read
     * @returns:
     *    - packet: the packet to populate
     *    - int: the number of bytes read from datas
     */
    const char* readCgroup (CgroupPacket & packet, const char* datas);
  
    /**
     * Transform a packet into raw datas 
     * @params:
     *   - packet: the packet to transform
     * @returns: raw datas, that can be transformed back
     */
    std::vector<char> writePacketToBytes (const Packet & packet);

    /**
     * Transform a packet into raw datas 
     * @params:
     *   - packet: the packet to transform
     * @returns: 
     *    - cache: raw datas, that can be transformed back
     */
    void writePacketToBytes (const Packet & packet, std::vector<char> & cache);

    /**
     * Write metrics at the end of a vector
     * @params:
     *   - metrics: the metrics to write
     * @returns:
     *   - result: the vector to populate
     */
    void writeMetricsToBytes (std::vector<char>& result, const std::vector<Metric> & metrics);

    /**
     * Write a cgroup to the end of a vector
     * @params:
     *    - packet: the packet to write
     * @returns:
     *    - result: the vector to populate
     */
    void writeCgroupToBytes (std::vector <char> & result, const CgroupPacket & packet);
}

/**
 * Pretty string for a packet
 */
std::ostream& operator<<(std::ostream& stream, const common::net::Packet & packet);
