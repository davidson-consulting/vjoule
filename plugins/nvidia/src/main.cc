
#include <iostream>
#include <vector>

#include "reader.hh"

#include <common/_.hh>



nvidia::NvmlReader __GLOBAL_NVML__;

extern "C" bool init (const common::utils::config::dict* cfg) {
    bool perCgroup = true;
    if (cfg != nullptr) { perCgroup = cfg-> getOr <bool> ("cgroup-consumption", true); }
    return __GLOBAL_NVML__.configure (perCgroup);
}

extern "C" void poll () {
    __GLOBAL_NVML__.poll ();
}

extern "C" uint32_t gpu_nb_devices () {
    return __GLOBAL_NVML__.getNbDevices ();
}

extern "C" void gpu_get_energy (float * energy) {
    for (uint32_t i = 0 ; i < __GLOBAL_NVML__.getNbDevices () ; i++) {
	energy[i] = __GLOBAL_NVML__.getGpuEnergy (i);
    }
}

extern "C" std::unordered_map<std::string, float> gpu_cgroup_usage (uint32_t device) {
    return __GLOBAL_NVML__.getDeviceUsage (device);
}

extern "C" void dispose () {
    __GLOBAL_NVML__.dispose (); 
}

extern "C" std::string help () {
    std::stringstream ss;
    ss << "nvidia (" << __PLUGIN_VERSION__ << ")" << std::endl;
    ss << __COPYRIGHT__ << std::endl << std::endl;

    ss << "Nvidia is a device plugin, it retreive the consumption of nvidia graphics card using nvml." << std::endl;
    ss << "It can be only be used for the component [gpu]." << std::endl;
    ss << "This plugin takes only one element of configuration 'cgroup-consumption'." << std::endl << std::endl;
    ss << "===" << std::endl;
    ss << "[gpu]" << std::endl;
    ss << "name = \"nvidia\"" << std::endl;
    ss << "cgroup-consumption = true" << std::endl;
    ss << "===" << std::endl << std::endl << std::endl;

    ss << "If 'cgroup-consumption' is true, then the plugin will retreive the name of the cgroups using the device, and their percentage of usage." << std::endl;
    ss << "Depending on the graphics card, cgroup usage can be available or not. Warning messages are displayed if it is not available." << std::endl;
    
    ss << "The plugin is capable of managing multiple devices, if multiple graphics card are found on the machine." << std::endl;
    return ss.str ();	
}
