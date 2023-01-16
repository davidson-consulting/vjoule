#include <iostream>
#include <vector>
#include <common/_.hh>

extern "C" bool init (const common::utils::config::dict * cfg) {
    
    return true;
}

extern "C" bool poll () {
    return true;
}

extern "C" void dump () {}

extern "C" void dispose () {}
