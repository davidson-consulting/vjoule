#pragma once

#include <string>

namespace tools::vjoule {
  class Watcher {
    public:
      Watcher(std::string path, std::string file);
      void wait();

    private:
      std::string _path;
      std::string _file;
  };
}
