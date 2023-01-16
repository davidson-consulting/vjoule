#include <iostream>
#include <vector>
#include <common/_.hh>

using namespace common;

utils::Logger __LOGGER__;

extern "C" bool init (const common::utils::config::dict * cfg) {
    if (cfg != nullptr) {
	auto newLogFile = cfg-> getOr <std::string> ("log-path", "");
	__LOGGER__.redirect (newLogFile, false);	
	__LOGGER__.changeLevel (cfg-> getOr<std::string> ("log-lvl", "SUCCESS"));
	__LOGGER__.setName ("divider");
    }

    
    return true;
}

extern "C" bool poll () {
    return true;
}

extern "C" void dispose () {}

extern "C" void dump (const std::ostream & s,
		      const std::vector <common::cgroup::Cgroup> & cgroups,
		      const std::vector <std::vector<uint64_t> > & perfCounters,
		      common::plugin::Factory & factory) {}

extern "C" std::vector <std::string> get_perf_events () {
    return {"PERF_COUNT_HW_CPU_CYCLES", "LLC_MISSES"};
}

extern "C" void compute (const std::vector <common::cgroup::Cgroup> & cgroups,
			 const std::vector <std::vector<uint64_t> > & perfCounters,
			 common::plugin::Factory & factory) {

    __LOGGER__.debug ("Formula compute");    
}
