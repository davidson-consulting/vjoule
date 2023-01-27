#pragma once

#include <common/concurrency/_.hh>
#include <vector>
#include <string>

namespace tools::vjoule {
  class VJoule {
    
    public:
      VJoule(int argc, char * argv[]);
      void run();

    private:
      int _argc;
      char ** _argv;
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
