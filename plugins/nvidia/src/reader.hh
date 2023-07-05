#pragma once

#define __PLUGIN_VERSION__ "1.2.0"
#define __PROJECT__ "NVIDIA"

#include <common/_.hh>
#include <nvml.h>

namespace nvidia {


    class NvmlReader {
    private: 

	std::unordered_map <uint64_t, std::string> _savedPids;

	std::vector <std::unordered_map <std::string, float> > _cgroupUsage;

	std::vector <nvmlDevice_t> _devices;

	std::vector <float> _deviceEnergy;

	std::vector <uint32_t> _deviceLastPower;

	std::vector <std::string> _deviceNames;

	std::vector <bool> _cgroupCapable;

	common::concurrency::timer _timer;
	
    public:

	/**
	 * Configure the nvml reading
	 * @params: 
	 *    - perCgroup: if true enable cgroup usage computation
	 */
	bool configure (bool perCgroup);
	
	/**
	 * Poll the energy consumption of the devices
	 * Also poll the cgroup usage if available
	 */
	void poll ();
	
	/**
	 * @returns: the number of nvidia devices found 
	 */
	uint32_t getNbDevices () const;

	/**
	 * @returns: the energy consumption of the device 'device'
	 */
	float getGpuEnergy (uint32_t device) const;

	/**
	 * @returns: the cgroup usage of a given device
	 */
	const std::unordered_map <std::string, float> & getDeviceUsage (uint32_t device) const;
	
	/**
	 * Dispose the handles of the reader
	 */
	void dispose ();


    private :

	/**
	 * @returns: the name of the cgroup of a given pid
	 */
	std::string getCgroupName (uint64_t pid);

	/**
	 * compute the energy consumption of a device between two poll
	 */
	float computeEnergy (uint32_t device, uint32_t currentPower, float elapsed);
	
    };


}
