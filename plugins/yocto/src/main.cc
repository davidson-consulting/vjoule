#include <iostream>
#include <vector>
#include <common/_.hh>

#include "reader.hh"


extern "C" bool init (const common::utils::config::dict* d) {
  return yocto::YoctoReader::__GLOBAL_Yocto__.configure (d);
}

extern "C" void poll () {
  yocto::YoctoReader::__GLOBAL_Yocto__.poll ();
}

extern "C" float pdu_get_energy () {
  return yocto::YoctoReader::__GLOBAL_Yocto__.getEnergy ();
}

extern "C" float pdu_get_power () {
  return yocto::YoctoReader::__GLOBAL_Yocto__.getPower ();
}

extern "C" void dispose () {
  yocto::YoctoReader::__GLOBAL_Yocto__.dispose ();
}

extern "C" std::string help () {
  std::stringstream ss;
  ss << "yocto (" << __PLUGIN_VERSION__ << ")" << std::endl;
  ss << __COPYRIGHT__ << std::endl << std::endl;

  ss << "Yocto is a device plugin that measures power consumption using a YoctoWatt PDU." << std::endl;
  ss << "It can be used for the [pdu] component." << std::endl;
  ss << "This plugin takes only one element of configuration 'target'." << std::endl << std::endl;
  ss << "===" << std::endl;
  ss << "[gpu]" << std::endl;
  ss << "name = \"nvidia\"" << std::endl;
  ss << "target = \"YWATTMK1-276146\"" << std::endl;
  ss << "===" << std::endl << std::endl << std::endl;
  ss << "This configuration is optional, if disabled, the first YoctoWatt device found will be used." << std::endl;

  return ss.str ();
}
