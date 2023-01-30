#pragma once
#include <optional>
#include <iostream>
#include <vector>
#include <string>

namespace tools::vjoule {
  struct Values {
    std::optional<float> cpu;
    std::optional<float> gpu;
    std::optional<float> ram;
  };

  std::ostream& operator<< (std::ostream& os, const Values& v);

  class Exporter {
    public:
      Exporter(std::string result_dir);
      // export result summary to stdout
      void export_stdout();

    private:
      // path to the vjoule result directory
      std::string _result_dir;
      
      std::vector<std::string> list_processes();
      Values read_for_process(std::string path);
  };
}
