#pragma once

#include <common/concurrency/_.hh>
#include <common/cgroup/_.hh>
#include "command.hh"
#include <vector>
#include <string>

namespace tools::vjoule {
    
  class VJoule {
  private :

      // The command line passed by command parser
      CommandLine _cmd;

      // The directory for output
      std::string _vjoule_directory;

      // The path of the config file of the service launched
      std::string _cfg_path;

      // The path of the current directory
      std::string _working_directory;

      // The sub process launched and watched
      common::concurrency::SubProcess _child;

      // The cgroup in which the child process will be attached
      common::cgroup::Cgroup _cgroup;
      
    public:

      /**
       * @params:
       *   - cmd: the command line parsed by the command parser
       */
      VJoule(const CommandLine & cmd);

      /**
       * Start the sub process, and watch its consumption
       */
      void run();

    private:

      /**
       * Run the parent process
       */
      void runParent (uint64_t childPid, int pipe);

      /**
       * Run the child process
       */
      void runChild (int pipe);

      /**
       * Create a default configuration for the sensor
       */
      void create_default_config();

      /**
       * Create the cgroups file for the sensor
       */
      void create_default_cgroups_list();

      /**
       * Configure the sensor 
       */
      void create_configuration();

      /**
       * Create the directory that will store the results
       */
      void create_result_directory();
  };
}
