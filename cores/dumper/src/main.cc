#include <iostream>
#include <vector>
#include "dumper.hh"
#include <common/_.hh>

dumper::Dumper __GLOBAL_DUMPER__;

extern "C" bool init (const common::utils::config::dict * cfg, common::plugin::Factory * factory) {
    return __GLOBAL_DUMPER__.configure (*cfg, *factory);	
}

extern "C" void compute () {
    __GLOBAL_DUMPER__.compute ();
}

extern "C" void dispose () {
    __GLOBAL_DUMPER__.dispose ();
}

extern "C" std::string help () {
    std::stringstream ss;
    ss << "dumper (" << __PLUGIN_VERSION__ << ")" << std::endl;
    ss << __COPYRIGHT__ << std::endl << std::endl;
    
    ss << "Dumper is a core plugin, it will append the energy consumption of the different components in a CSV file, along with the value of selected perf events for selected cgroups." << std::endl;
    ss << "Unlike the Divider core plugin, it will not divide the energy consumption of the components for each cgroups." << std::endl << std::endl;
    
    ss << "The following presents an example of a configuration, for the sensor that has to be placed in '/etc/vjoule/config.toml'." << std::endl << std::endl;
    ss << "=== config.toml : " << std::endl;
    ss << "[sensor]" << std::endl;
    ss << "freq = 1 # frequency of update in hertz (the higher the faster)" << std::endl;
    ss << "log-lvl = \"info\" # debug < success < info < warn < error < none" << std::endl;
    ss << "log-path = \"/etc/vjoule/log\" # log file (empty means stdout)" << std::endl;
    ss << "core = \"dumper\" # the name of the core plugin to use for the sensor" << std::endl;
    ss << "perf-counters = [\"LLC_MISSES\", \"PERF_COUNT_HW_CPU_CYCLES\"] # The list of performances counters to monitor" << std::endl;
    ss << std::endl;
    ss << "# following configuration is optional" << std::endl;
    ss << "# It activates some part of the dumper" << std::endl;
    ss << std::endl;
    ss << "[cpu] # configuration to enable CPU energy reading" << std::endl;
    ss << "name = \"rapl\" # rapl plugin for compatible intel or amd cpus" << std::endl;
    ss << std::endl;
    ss << "[ram] # configuration to enable RAM energy reading" << std::endl;
    ss << "name = \"rapl\" # rapl plugin for compatible intel or amd cpus" << std::endl;
    ss << std::endl;
    ss << "[gpu:0] # configuration to enable GPU energy reading" << std::endl;
    ss << "name = \"nvidia\" # nvidia plugin for nvidia GPUs" << std::endl;
    ss << std::endl;
    ss << "# A machine can have different GPU from different constructors" << std::endl;
    ss << "# For example a integrated GPU, and an nvidia GPU card" << std::endl;
    ss << "[gpu:1] # configuration to enable GPU energy " << std::endl;
    ss << "name = \"rapl\" # rapl plugin form compatible intel of amd cpus" << std::endl;
    ss << "===" << std::endl << std::endl;

    ss << "The dumper core plugin uses another configuration file, in '/etc/vjoule/cgroups', to filter the cgroups that are watched by the sensor, and ignore unwanted cgroups." << std::endl;
    ss << "This configuration file is a list of rules, every rules must start with a slice (a cgroup that contains other cgroups)." << std::endl;
    ss << "For example 'my.slice/*'., will make the sensor watch all the cgroups in the slice 'my.slice', and 'system.slice/docker*' will watch all the cgroups in 'system.slice' that starts with 'docker'." << std::endl;
    ss << "Basically, it list all the cgroups that would be found if the command 'ls my_rule' was run in the cgroup mount directory." << std::endl;
    ss << "The following presents an example of configuration for cgroup listing that has to be placed in '/etc/vjoule/cgroups'" << std::endl << std::endl; 
    ss << "=== cgroups : " << std::endl;
    ss << "vjoule.slice/*" << std::endl;
    ss << "system.slice/docker*" << std::endl;
    ss << "my_custom.slice/my_custom_cgroup" << std::endl;
    ss << "===" << std::endl;;
    
    return ss.str ();
}