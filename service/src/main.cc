#include <sensor/_.hh>
#include <common/_.hh>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <sys/wait.h>

using namespace common::utils;
using namespace common;


void ctrlCHandler (int signum) {
    ::sensor::exitSignal.emit ();
    
    exit (-1);
}

int main (int argc, char ** argv) {    
    signal(SIGINT, &ctrlCHandler);
    signal(SIGTERM, &ctrlCHandler);
    signal(SIGKILL, &ctrlCHandler);

    try {
	::sensor::Sensor s;
	s.configure (argc, argv);
	s.run ();
    } catch (...) {
	return -1;
    }
    
    return 0;
}
