#pragma once
#include <iostream>
#include <vector>
#include <string>
#include "command.hh"

namespace tools::vjoule {
  struct Values {
    float cpu;
    float gpu;
    float ram;

    Values operator - (const Values &v) const {
      return Values(
          cpu - v.cpu,
          gpu - v.gpu,
          ram - v.ram
        );
    }
  };

  class Exporter {
  private :

      // path to the vjoule result directory
      std::string _result_dir;

      // The name of the cgroup containing the interesting results
      std::string _cgroupName;

      // time to write in csv file (time of the experiment)
      std::string _time;
      
      // True if cpu have to be exported
      bool _cpu;

      // True if gpu have to be exported
      bool _gpu;

      // True if ram have to be exported
      bool _ram;

      // Initial result of global (before starting the child process)
      Values _initGlobal;

      // Initial result of process (before starting the child process)
      Values _initProcess;
      
    public:

      /**
       * Create an empty exporter
       */
      Exporter();

      /**
       * @params:
       *   - resultDir: the directory containing the result
       *   - cgroupName: the name of the cgroup created by the CLI
       *   - cmd: the options passed to the CLI 
       */
      void configure (const std::string & resultDir, const std::string & cgroupName, const CommandLine & cmd, std::string time);
      
      /**
       * export result summary to stdout
       */
      void export_stdout();

      /**
       * export result summary to a csv file
       */
      void export_csv(const std::string & output_file);

    private:
      
      Values read_for_process(const std::string & path);
      
      std::string value_stdout(Values v);
      std::string value_csv(Values v);
  };
}
