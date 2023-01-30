#pragma once
#include <iostream>
#include <vector>
#include <string>

namespace tools::vjoule {
  struct Values {
    float cpu;
    float gpu;
    float ram;
  };

  class Exporter {
    public:
      Exporter(std::string result_dir, bool cpu, bool gpu, bool ram);
      // export result summary to stdout
      void export_stdout();
      void export_csv(std::string output_file);

    private:
      // path to the vjoule result directory
      std::string _result_dir;
      bool _cpu;
      bool _gpu;
      bool _ram;
      
      std::vector<std::string> list_processes();
      Values read_for_process(std::string path);
      std::string value_stdout(Values v);
      std::string value_csv(Values v);
  };
}
