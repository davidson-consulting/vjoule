#pragma once

#include <common/_.hh>
#include <common/concurrency/_.hh>
#include "command.hh"
#include <vjoule_api.hh>
#include <vector>
#include <string>

namespace tools::vjoule {

    struct ExecError {};

    class Exec {
    private :

		::vjoule::vjoule_api _api;

		// The command line passed by command parser
		CommandLine _cmd;

		// The sub process launched and watched
		common::concurrency::SubProcess _child;

		// The pid of the child
		uint64_t _childPid = 0;

		// The sensor service is started
		bool _started = false;

    public:

		/**
		 * @params:
		 *   - cmd: the command line parsed by the command parser
		 */
		Exec(const CommandLine & cmd);

		/**
		 * Start the sub process, and watch its consumption
		 */
		void run();

		/**
		 * Dispose everything that was created by the exec command
		 */
		void dispose ();

    private:

		/**
		 * Run the parent process
		 */
		void runParent (uint64_t childPid, int pipe);

		/**
		 * Run the child process
		 */
		void runChild (int pipe);


    };
}
