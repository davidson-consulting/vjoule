#include <common/concurrency/_.hh>

#include <string>
#include <iostream>
#include <unistd.h>
#include <libcgroup.h>
#include <sys/wait.h>

#include <rapl.hh>
#include <vjoule.hh>
#include <watch.hh>
#include <vjoule_exec.hh>

namespace tools::vjoule
{

	VJoule::VJoule(int argc, char *argv[]) : _argc(argc),
											 _argv(argv)
	{
		if (getuid())
		{
			std::cerr << "You are not root. This program will only work if run as root." << std::endl;
			exit(-1);
		}

		if (argc < 2)
		{
			std::cerr << "Usage: vjoule cmd [options...]" << std::endl;
			exit(-1);
		}

		std::vector<std::string> subargs;
		for (int i = 2; i < argc; i++)
		{
			subargs.push_back(argv[i]);
		}

		this->_child = common::concurrency::SubProcess(argv[1], subargs, ".");

		// create cgroup for running vjoule_exec
		int ret = 0;
		ret = cgroup_init();

		if (ret)
		{
			fprintf(stderr, "cgroup_init failed\n");
			exit(1);
		}

		struct cgroup *cg = NULL;
		cg = cgroup_new_cgroup("vjoule_xp.slice/process");
		if (!cg)
		{
			fprintf(stderr, "Failed to allocate cgroup %s\n", "vjoule_xp.slice/process");
			exit(1);
		}

		struct cgroup_controller *cgc;
		cgc = cgroup_add_controller(cg, "cpu");
		if (!cgc)
		{
			printf("FAIL: cgroup_add_controller failed\n");
			exit(3);
		}

		ret = cgroup_create_cgroup(cg, 0);
		if (ret)
		{
			printf("FAIL: cgroup_create_cgroup failed with %s\n",
				   cgroup_strerror(ret));
			exit(3);
		}
	}

	void VJoule::run()
	{
		pfm_initialize();

		// get perf counters & energy consumption
		std::vector<std::string> eventList = {"PERF_COUNT_HW_CPU_CYCLES", "LLC_MISSES"};

		PerfEventWatcher systemWatcher = PerfEventWatcher("/sys/fs/cgroup");
		systemWatcher.configure(eventList);
		PerfEventWatcher cgWatcher = PerfEventWatcher("/sys/fs/cgroup/vjoule_xp.slice");
		cgWatcher.configure(eventList);

		RaplReader raplReader = RaplReader();
		common::net::Packet raplValues;

		// std::cout << cgWatcher.poll() << std::endl;
		// std::cout << systemWatcher.poll() << std::endl;
		// std::cout << raplValues << std::endl;

		pid_t c_pid = fork();

		if (c_pid == -1)
		{
			printf("Could not fork to execute vjoule_exec");
			exit(EXIT_FAILURE);
		}
		else if (c_pid > 0)
		{
			// parent process
			int status;
			wait(&status);

			std::vector<long long> perfCountersCG = cgWatcher.poll();
			std::vector<long long> perfCountersSystem = systemWatcher.poll();
			raplReader.poll(raplValues);

			std::cout << "== Memory == " << std::endl;
			float energyDramCG = raplValues.energy_dram * ((double)perfCountersCG[1] / (double)perfCountersSystem[1]);
			std::cout << "Energy consumed by machine " << raplValues.energy_dram << "j - Energy consummed by process " << energyDramCG << "j (" << energyDramCG / raplValues.energy_dram * 100 << "\% of total)" << std::endl;

			std::cout << "== Package == " << std::endl;
			float energyPackageCG = raplValues.energy_pkg * ((double)perfCountersCG[0] / (double)perfCountersSystem[0]);
			std::cout << "Energy consumed by machine " << raplValues.energy_pkg << "j - Energy consummed by process " << energyPackageCG << "j (" << energyPackageCG / raplValues.energy_pkg * 100 << "\% of total)" << std::endl;

			pfm_terminate();
		}
		else
		{
			// child process
			vjoule_exec("vjoule_xp.slice/process", this->_child);
		}
	}
}
