#include <iostream>

#include <vjoule.hh>

int main(int argc, char * argv[]) {
  tools::vjoule::VJoule vjoule(argc, argv);
  vjoule.run();
  return 0;
}
