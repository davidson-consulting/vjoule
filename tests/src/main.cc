#include <iostream>

#include <campaign.hh>

using namespace tests;
int main(int argc, char * argv[]) {
  std::cout << "Running tests..." << std::endl;
  std::cout << "Test directory: " << argv[1] << std::endl;

  Campaign campaign(argv[1]);
  campaign.run();
}
