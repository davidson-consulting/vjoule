#pragma once

#include <common/utils/_.hh>
#include <common/plugin/factory.hh>
#include <cstdint>

namespace common::plugin {

    typedef void (*CoreComputeFunc_t)();
    typedef bool (*CoreInitFunc_t) (const common::utils::config::dict* cfg,
				    common::plugin::Factory* factory);

    typedef void (*PluginDisposeFunc_t) ();
    typedef void (*PluginPollFunc_t) ();

    typedef float (*CpuGetEnergy_t) ();
    typedef float (*RamGetEnergy_t) ();
    
    typedef uint32_t (*GpuGetNbDevices_t) ();
    typedef void (*GpuGetEnergy_t) (float* power);
    typedef float (*GpuGetDeviceUsage_t) (uint32_t device, const char * cgroupName);

    typedef std::string (*HelpFunc_t) ();
}
