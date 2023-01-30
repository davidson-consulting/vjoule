#pragma once

#include <common/concurrency/_.hh>
#include "command.hh"
#include <vector>
#include <string>

namespace tools::vjoule {
  class VJoule {
    
    public:
      VJoule(const CommandLine & cmd);
      void run();

    private:
      
      CommandLine _cmd;
      
      std::string _vjoule_directory;
      std::string _cfg_path;
      std::vector<std::string> subargs;
      common::concurrency::SubProcess _child;

      void print_help();
      void create_default_config();
      void create_default_cgroups_list();
      void create_configuration_if_needed();
      void create_result_directory();
  };
}
