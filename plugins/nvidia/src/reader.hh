#pragma once

#define __PLUGIN_VERSION__ "1.3.0"
#define __PROJECT__ "NVIDIA"

#include <common/_.hh>
#include <nvml.h>

namespace nvidia {


    class NvmlReader {
    private:

		std::vector <nvmlDevice_t> _devices;

		std::vector <float> _deviceEnergy;

		std::vector <uint32_t> _deviceLastPower;

		std::vector <std::string> _deviceNames;

		common::concurrency::timer _timer;
	
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


    private :

		/**
		 * compute the energy consumption of a device between two poll
		 */
		float computeEnergy (uint32_t device, uint32_t currentPower, float elapsed);
	
    };


}
