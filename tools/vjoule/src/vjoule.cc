#include <optional>

#include "vjoule.hh"

using namespace tools::vjoule;

enum FLAGS {
  HELP,
  OUTPUT_DIR,
};

VJoule::VJoule(int argc, char * argv[]): _argc(argc), _argv(argv) {
  if (getuid()) {
    std::cerr << "You are not root. This program will only work if run as root." << std::endl;
    exit(-1);
  }

  if (argc < 2) {
    this-> print_help();
	  exit(-1);
  }

  this-> parse_arguments();
}

void VJoule::parse_arguments() {
  int i = 0;
  do {

  } while (i < this-> _argc);
}

std::optional<FLAGS> VJoule::next_flag(int i, argv) {
  if(argv[i] )
}

void VJoule::print_help() {
  std::cerr << "Usage: vjoule cmd [options...]" << std::endl;
}

void VJoule::run() {
  // vjoule # print help
  // vjoule --help # print help
  // vjoule python monscript.py # output dir = tmp
  // vjoule --output-dir /my/dir python monscript.py
  // output dir : directory with config used and sensor results
}
