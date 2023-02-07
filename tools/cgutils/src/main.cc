#include <common/_.hh>
#include <common/foreign/CL11.hpp>
#include <cstdint>
#include <iostream>

void evaluateCreate (const std::string & cgroup_name) {
    common::cgroup::Cgroup c (cgroup_name);
    c.create ();
}

void evaluateRemove (const std::string & cgroup_name) {
    common::cgroup::Cgroup c (cgroup_name);
    c.detachAll ();
    c.remove ();
}

void evaluateAttach (const std::string & cgroup_name, const std::vector <uint64_t> & pids) {
    common::cgroup::Cgroup c (cgroup_name);
    c.create ();
    for (auto & pid : pids) {
	c.attach (pid);
    }    
}

int main (int argc, char ** argv) {
    CLI::App app {"vjoule_cgutils"};
    CLI::App * create = app.add_subcommand ("add", "create a cgroup");
    CLI::App * remove = app.add_subcommand ("del", "delete a cgroup");
    CLI::App * attach = app.add_subcommand ("attach", "delete a cgroup");

    std::string cgroup_name;
    create-> add_option ("cgroup", cgroup_name, "name of the cgroup to create")-> required ();
    remove-> add_option ("cgroup", cgroup_name, "name of the cgroup to remove")-> required ();
    attach-> add_option ("cgroup", cgroup_name, "name of the cgroup to remove")-> required ();

    std::vector <uint64_t> pids;
    attach-> add_option ("pids", pids, "pids to attach to the cgroup")-> required ();
    
    CLI11_PARSE(app, argc, argv);
    
    if (create-> parsed ()) {
	evaluateCreate (cgroup_name);
    } else if (remove-> parsed ()) {
	evaluateRemove (cgroup_name);
    } else if (attach-> parsed ()) {
	evaluateAttach (cgroup_name, pids);
    } else {
	std::cerr << "vjoule_cgutils (add | del | attach)" << std::endl;
    }    
}
