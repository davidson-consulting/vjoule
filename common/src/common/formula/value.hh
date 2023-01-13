#pragma once
#include <iostream>

namespace common::formula {
    
    struct PackageValue {
	double package;
	double pp0;
	double pp1;
	double dram;
	double psys;
    };
    
}

std::ostream& operator<< (std::ostream& s, common::formula::PackageValue p);
