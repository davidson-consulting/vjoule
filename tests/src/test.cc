#include <common/_.hh>
#include <test.hh>

using namespace common;

namespace tests {
  Test::Test(std::string cfgPath) {
    auto cfg = utils::parse_file (cfgPath);
    this-> configure(cfg);

  }

  void Test::configure(utils::config::dict & cfg) {
    auto cTest = cfg.getOr <utils::config::dict> ("test", {});
    this-> _cmd = cTest.getOr <std::string> ("cmd", "");
    this-> _hostCmd = cTest.getOr <std::string> ("host.cmd", "");

    auto cOutput = cfg.getOr <utils::config::dict> ("output", {});
    this-> _output = cOutput.getOr <std::string> ("csv", "results.csv");

    auto cCompare = cfg.getOr <utils::config::dict> ("compare", {});
    this-> _expected = cCompare.getOr <std::string> ("csv", "expected.csv");
    this-> _tolerance = cCompare.getOr <int> ("tolerance", 10);

  }

  std::string Test::string() {
    std::stringstream ss;
    ss << "Test(cmd=\""; 
    ss << this-> _cmd;
    ss << "\", host-cmd=\"";
    ss << this-> _hostCmd;
    ss << "\")";

    return ss.str();
  }
}
