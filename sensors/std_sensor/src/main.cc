#include <common/_.hh>
#include <sensor/_.hh>
#include <stdlib.h>
#include <unistd.h>
#include <asm/msr.h>
#include <iostream>


using namespace common::utils;
using namespace common;

int main (int argc, char ** argv) {    
    if (getuid ()) {
	utils::Logger::globalInstance ().error ("You are not root. This program will only work if run as root.");
	exit (-1);
    }
    
    ::sensor::Sensor sensor (argc, argv);
    sensor.run ();

    return 0;
}
