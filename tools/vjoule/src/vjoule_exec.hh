#pragma once

#include <libcgroup.h>
#include <unistd.h>
#include <common/concurrency/_.hh>

namespace tools::vjoule {
    void vjoule_exec(const char* cgpath, common::concurrency::SubProcess child) {
        // attach itself to cgroup
        int ret = cgroup_change_cgroup_path(cgpath, getpid(), (const char * const []){"cpu"});
		if (ret) {
			printf("cgroup change of group failed\n");
			exit(ret);
		}

        child.start();
        child.wait();
    }
}