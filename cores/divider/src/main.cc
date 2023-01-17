#include <iostream>
#include <vector>
#include "divider.hh"
#include <common/_.hh>


divider::Divider __GLOBAL_DIVIDER__;

extern "C" bool init (const common::utils::config::dict * cfg, common::plugin::Factory * factory) {
    return __GLOBAL_DIVIDER__.configure (*cfg, *factory);	
}

extern "C" void compute () {
    __GLOBAL_DIVIDER__.compute ();
}

extern "C" void dispose () {
    __GLOBAL_DIVIDER__.dispose ();
}
