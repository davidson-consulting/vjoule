#include "reader.hh"
#include <math.h>

using namespace common;


namespace nvidia {

    bool NvmlReader::configure (bool perCgroups) {
	if (!perCgroups) {
	    LOG_INFO ("Cgroup consumption disabled.");
	} else {
	    LOG_INFO ("Cgroup consumption enabled.");

	    bool v2 = false;
	    std::string cgroupRoot = utils::get_cgroup_mount_point (v2);
	    if (!v2) {
		LOG_ERROR ("Cgroup v2 not mounted, nvidia plugin only supports cgroup v2");
		return false;
	    }	    
	}
	
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

	    if (perCgroups) {
		nvmlProcessUtilizationSample_t utilization[255];
		unsigned int processSampleCount;
		auto r = nvmlDeviceGetProcessUtilization (device, NULL, &processSampleCount, 0);
		r = nvmlDeviceGetProcessUtilization (device, utilization, &processSampleCount, 0);
	    
		if (r != NVML_SUCCESS) {
		    LOG_WARN ("Device '", name, "' cannot retreive cgroup usage.");
		    this-> _cgroupCapable.push_back (false);
		} else {
		    this-> _cgroupCapable.push_back (true);
		}
	    } else this-> _cgroupCapable.push_back (false);
	    
	    this-> _cgroupUsage.push_back ({});
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
	    if (this-> _cgroupCapable [i]) {
		this-> _cgroupUsage [i].clear ();
		nvmlProcessUtilizationSample_t utilization[255];
		unsigned int processSampleCount;
		nvmlDeviceGetProcessUtilization (this-> _devices[i], NULL, &processSampleCount, 0);
		nvmlDeviceGetProcessUtilization (this-> _devices[i], utilization, &processSampleCount, 0);

		for (uint64_t j = 0 ; j < processSampleCount ; j++) {
		    auto usage = utilization[j].smUtil + utilization[j].encUtil + utilization[j].decUtil;
		    if (usage > 0) {
			auto name = this-> getCgroupName (utilization[j].pid);
			savedPid.emplace (utilization[j].pid, name);
			this-> _cgroupUsage [i][name] += (usage / 100.0); // between 0 and 1
		    }
		}		
	    }
	}

	this-> _savedPids = std::move (savedPid);
    }


    const std::unordered_map <std::string, float> & NvmlReader::getDeviceUsage (uint32_t device)  const {
	return this-> _cgroupUsage[device];
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

    std::string NvmlReader::getCgroupName (uint64_t pid) {
	auto it = this-> _savedPids.find (pid);
	if (it != this-> _savedPids.end ()) {
	    return it-> second;
	}

	bool v2 = false;
	std::string cgroupRoot = utils::get_cgroup_mount_point (v2);
	
	std::ifstream t("/proc/" + std::to_string (pid) + "/cgroup");
	std::stringstream buffer;
	buffer << t.rdbuf();
	if (buffer.str ().size () > 3) {
	    auto str = buffer.str ();
	    return common::utils::join_path (cgroupRoot, str.substr (3, str.find ("\n") - 3));
	} else return "";
    }

    
}
