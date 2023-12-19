#include <iostream>
#include <vector>
#include <common/_.hh>

#include "reader.hh"


yocto::YoctoReader __GLOBAL_Yocto__;

extern "C" bool init (const common::utils::config::dict* d) {
  return __GLOBAL_Yocto__.configure (d);
}

extern "C" void poll () {
  __GLOBAL_Yocto__.poll ();
}

extern "C" float pdu_get_energy () {
  return __GLOBAL_Yocto__.getEnergy ();
}

extern "C" void dispose () {
  __GLOBAL_Yocto__.dispose ();
}

extern "C" std::string help () {
  std::stringstream ss;
  ss << "HELP YOCTO" << std::endl;
  return ss.str ();
}
