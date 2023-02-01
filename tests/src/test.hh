#pragma once

#include <iostream>
#include <string>

#include <common/utils/config.hh>

namespace tests {
  class Test {
    public:
      /*
       * Create Test from TOML config file
       */ 
      Test(std::string cfgPath);

      /*
       * Run the test, print results
       * @return a boolean that indicates if test succedded or not
       */
      bool run();

      /*
       * Return a string representing the test
       */
      std::string string();

      private:
        // command to execute to run test
        std::string _cmd;

        // command to execute on host while running test (if needed)
        std::string _hostCmd;

        // path to file output
        std::string _output;

        // path to expected result output
        std::string _expected;
        
        // tolerance in percentage between output and expected results
        int _tolerance;

        void configure(common::utils::config::dict & cfg);
  };

};
