#include <iostream>
#include <vector>
#include <common/_.hh>

#include "reader.hh"


rapl::RaplReader __GLOBAL_RAPL__;

extern "C" bool init (const common::utils::config::dict*) {    
    return __GLOBAL_RAPL__.configure ();
}

extern "C" void poll () {
    __GLOBAL_RAPL__.poll ();
}

extern "C" float cpu_get_energy () {
    return __GLOBAL_RAPL__.getCpuEnergy ();
}

extern "C" float ram_get_energy () {
    return __GLOBAL_RAPL__.getRamEnergy ();
}

extern "C" uint32_t gpu_nb_devices () {
    return 1;
}

extern "C" void gpu_get_energy (float * energy) {
    energy[0] = __GLOBAL_RAPL__.getGpuEnergy ();
}

extern "C" void dispose () {
    __GLOBAL_RAPL__.dispose (); 
}

extern "C" std::string help () {
    std::stringstream ss;
    ss << "rapl (" << __PLUGIN_VERSION__ << ")" << std::endl;
    ss << __COPYRIGHT__ << std::endl << std::endl;

    ss << "Rapl is a device plugin, it retreive the consumption a CPU, RAM and integrated GPU using RAPL msr." << std::endl;
    ss << "It can be used for the three components [cpu], [gpu] and [ram]." << std::endl;
    ss << "There is no specific configuration to pass to the components in the configuration file." << std::endl;
    ss << "Depending on the machine, GPU and RAM may be unavailable, in which case warnings will be displayed during configuration, and RAM and GPU power consumption will always return 0." << std::endl << std::endl;

    return ss.str ();
}
