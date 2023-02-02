#include <sensor/_.hh>
#include <common/_.hh>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>


using namespace common::utils;
using namespace common;

int main (int argc, char ** argv) {    
    try {
	::sensor::Sensor s (argc, argv);
	s.run ();
    } catch (...) {
	return -1;
    }
    
    return 0;
}
