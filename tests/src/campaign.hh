#pragma once

#include <vector>

#include <test.hh>

namespace tests {
  class Campaign {
    public:
      Campaign(std::string testsDir);
      void run();
    private:
      std::string _testsDir;

      std::vector<Test> _tests;
  };
}
