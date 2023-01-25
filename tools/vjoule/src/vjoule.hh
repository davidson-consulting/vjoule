#pragma once

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
      char ** _output_dir;
      std::vector<std::string> subargs;
      common::concurrency::SubProcess _child;

      void parse_arguments();
      void print_help();
  };
}
