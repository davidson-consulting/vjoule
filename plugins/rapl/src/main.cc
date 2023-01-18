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

extern "C" void dispose () {
    __GLOBAL_RAPL__.dispose (); 
}

extern "C" std::string help () {
    std::stringstream ss;
    ss << "ralp (" << __PLUGIN_VERSION__ << ")" << std::endl;
    ss << __COPYRIGHT__ << std::endl << std::endl;

    ss << "Rapl is a device plugin, it retreive the consumption a CPU, RAM and integrated GPU using RAPL msr." << std::endl;
    ss << "It can be used for the three components [cpu], [gpu] and [ram]." << std::endl;
    ss << "There is no specific configuration to pass to the components in the configuration file." << std::endl;
    ss << "Depending on the machine, gpu and ram may be unavailable, in that case during the configuration warnings will be displayed, and ram and gpu energy consumption retreiving will always return 0." << std::endl;
    ss << "GPU number of available devices is always lower or equal to 1." << std::endl;

    return ss.str ();	
}
