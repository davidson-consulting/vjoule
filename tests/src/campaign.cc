#include <filesystem>

#include <campaign.hh>

namespace tests {
  Campaign::Campaign(std::string testsDir): _testsDir(testsDir) {
    // generate every tests from files
    // TODO refactor and use a separate config class for test creation from file
    for (auto const& entry: std::filesystem::directory_iterator(this-> _testsDir)) {
      if(!std::filesystem::is_regular_file(entry)) continue;

      this-> _tests.push_back(Test(entry.path().string()));
    }
  }

  void Campaign::run() {
    for (auto t: this-> _tests) {
      std::cout << t.string() << std::endl;
    }
  }
}
