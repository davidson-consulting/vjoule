#include <common/formula/value.hh>

std::ostream& operator<< (std::ostream& s, common::formula::PackageValue p) {
    s << "{" << "pkg : " << p.package << ", pp0 : " << p.pp0 << ", pp1 : " << p.pp1 << ", dram : " << p.dram << ", psys : " << p.psys << "}";

    return s;
}
