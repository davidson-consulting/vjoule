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
