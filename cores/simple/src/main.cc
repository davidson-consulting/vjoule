#include <iostream>
#include <vector>
#include "simple.hh"
#include <common/_.hh>


simple::Simple __GLOBAL_SIMPLE__;

extern "C" bool init (const common::utils::config::dict * cfg, common::plugin::Factory * factory) {
    return __GLOBAL_SIMPLE__.configure (*cfg, *factory);
}

extern "C" void compute () {
    __GLOBAL_SIMPLE__.compute ();
}

extern "C" void dispose () {
    __GLOBAL_SIMPLE__.dispose ();
}

extern "C" std::string help () {
    std::stringstream ss;
    ss << "simple (" << __PLUGIN_VERSION__ << ")" << std::endl;
    ss << __COPYRIGHT__ << std::endl << std::endl;

    ss << "Simple is a core plugin, it will append the energy consumption of the different components in a CSV file, along with the value of selected perf events for selected cgroups." << std::endl;
    ss << "Unlike the Divider core plugin, it will not divide the energy consumption of the components for each cgroups." << std::endl << std::endl;

    ss << "The following presents an example of a configuration, for the sensor that has to be placed in '/etc/vjoule/config.toml'." << std::endl << std::endl;
    ss << "=== config.toml : " << std::endl;
    ss << "[sensor]" << std::endl;
    ss << "freq = 1 # frequency of update in hertz (the higher the faster)" << std::endl;
    ss << "log-lvl = \"info\" # debug < success < info < warn < error < none" << std::endl;
    ss << "log-path = \"/etc/vjoule/log\" # log file (empty means stdout)" << std::endl;
    ss << "core = \"simple\" # the name of the core plugin to use for the sensor" << std::endl;

    ss << "# the directory in which result will be written" << std::endl;
    ss << "output-dir = \"/etc/vjoule/results\""<< std::endl << std::endl;

    ss << "# delete cgroup result directories when the cgroup no longer exists" << std::endl;
    ss << "delete-res = true" << std::endl << std::endl;

    ss << "# if true mount the result directory in tmpfs (less i/o generated by vjoule)" << std::endl;
    ss << "mount-tmpfs = true" << std::endl;
    ss << std::endl;

    ss << "# following configuration is optional" << std::endl;
    ss << "# It activates some part of the simple" << std::endl;
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
    ss << std::endl;
    ss << "[pdu] #configuration to enable smart PDU reading" << std::endl;
    ss << "name = \"yocto\" # the yocto plugin to read YoctoWatt PDU consumption" << std::endl;
    ss << "===" << std::endl << std::endl;

    return ss.str ();
}
