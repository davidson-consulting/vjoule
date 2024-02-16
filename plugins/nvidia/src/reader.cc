#include "reader.hh"
#include <math.h>

using namespace common;


namespace nvidia {

    bool NvmlReader::configure () {
		if (nvmlInit () != NVML_SUCCESS) {
			LOG_ERROR ("NVML is not available.");
			return false;
		}

		uint32_t nbDevices = 0;
		if (nvmlDeviceGetCount (&nbDevices) != NVML_SUCCESS) {
			LOG_ERROR ("NVML failed to get number of devices.");
			return false;
		}


		for (uint32_t i = 0 ; i < nbDevices ; i++) {
			nvmlDevice_t device;
			if (nvmlDeviceGetHandleByIndex (i, &device) != NVML_SUCCESS) {
				LOG_WARN ("NVML failed to retreive device ", i);
				continue;
			}

			char name [NVML_DEVICE_NAME_BUFFER_SIZE];
			if (nvmlDeviceGetName (device, name, NVML_DEVICE_NAME_BUFFER_SIZE) != NVML_SUCCESS) {
				LOG_WARN ("NVML failed to retreive device ", i);
				continue;
			}

			LOG_INFO ("Found nvidia device '", name , "'");
			this-> _devices.push_back (device);
			this-> _deviceNames.push_back (name);
			this-> _deviceEnergy.push_back (0);
			this-> _deviceLastPower.push_back (0);
		}
      
		return true;
    }

    void NvmlReader::poll () {
		float time = this-> _timer.time_since_start ();
		this-> _timer.reset ();

		std::unordered_map <uint64_t, std::string> savedPid;

		for (uint32_t i = 0 ; i < this-> _devices.size () ; i++) {
			uint32_t power = 0;
			nvmlDeviceGetPowerUsage (this-> _devices[i], &power);
			this-> _deviceEnergy[i] = this-> computeEnergy (i, power, time);
		}
    }
    
    float NvmlReader::computeEnergy (uint32_t device, uint32_t power, float time) {
		auto lastpower = this-> _deviceLastPower[device];
		this-> _deviceLastPower[device] = power;
		auto min = (power > lastpower) ? lastpower : power;
		auto max = (power > lastpower) ? power : lastpower;
	
		auto minW = min / 1000.0f;
		auto maxW = max / 1000.0f;
	
		float base = minW * time;
		float b = maxW - minW;
		float area = (time * b) / 2;

		return base + area;
    }

    
    uint32_t NvmlReader::getNbDevices () const {
		return 1;
    }

    float NvmlReader::getGpuEnergy (uint32_t device) const {
		return this-> _deviceEnergy[device];
    }

    void NvmlReader::dispose () {}

    
}
