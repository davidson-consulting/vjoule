#pragma once

#define __PLUGIN_VERSION__ "1.0"
#define __PROJECT__ "NVIDIA"

#include <common/_.hh>


namespace nvidia {


    class NvmlReader {
    private: 
	

    public:

	/**
	 * Configure the nvml reading
	 */
	bool configure ();
	
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
	 * Dispose the handles of the reader
	 */
	void dispose ();


    };


}
