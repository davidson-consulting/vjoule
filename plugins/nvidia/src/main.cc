#include <iostream>
#include <vector>

#include "reader.hh"

#include <common/_.hh>



nvidia::NvmlReader __GLOBAL_NVML__;

extern "C" bool init (const common::utils::config::dict*) {    
    return __GLOBAL_NVML__.configure ();
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

extern "C" float gpu_cgroup_usage (uint32_t, const char*) {
    return 0;
}

extern "C" void dispose () {
    __GLOBAL_NVML__.dispose (); 
}

extern "C" std::string help () {
    std::stringstream ss;
    ss << "nvidia (" << __PLUGIN_VERSION__ << ")" << std::endl;
    ss << __COPYRIGHT__ << std::endl << std::endl;

    ss << "Nvidia is a device plugin, it retreive the consumption nvidia graphics card using nvml." << std::endl;
    ss << "It can be only be used for the component [gpu]." << std::endl;
    ss << "There is no specific configuration to pass to the component in the configuration file." << std::endl;
    ss << "The plugin is capable of managing multiple devices, if multiple graphics card are found on the machine." << std::endl;
    ss << "Depending on the graphics card, cgroup usage can be available or not. Warning messages are displayed if it is not available." << std::endl;
    return ss.str ();	
}