#include <sensor/_.hh>
#include <common/_.hh>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>


using namespace common::utils;
using namespace common;

int main (int argc, char ** argv) {    
    if (getuid ()) {	
	LOG_ERROR ("You are not root. This program will only work if run as root.");
	exit (-1);
    }

    ::sensor::Sensor s (argc, argv);
    s.run ();
    
    return 0;
}
