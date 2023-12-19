#include "reader.hh"

#include <iostream>
#include <stdlib.h>
#include <ctime>


namespace yocto {

  bool YoctoReader::configure (const common::utils::config::dict * config) {
    std::string target = config-> getOr<std::string> ("target", "any");
    std::string errmsg;

    if (YAPI::RegisterHub ("usb", errmsg) != YAPI::SUCCESS) {
      LOG_ERROR ("Register USB error ", errmsg);
      return false;
    }

    if (target == "any") {
      this-> _psensor = YPower::FirstPower ();
      if (this-> _psensor == nullptr) {
        LOG_ERROR ("No YoctoWatt connected (check usb cable)");
        return false;
      }
    } else {
      this-> _psensor = YPower::FindPower (target + ".Power");
      if (this-> _psensor == nullptr) {
        LOG_ERROR ("Target ", target, " not found (check usb cable)");
        return false;
      }
    }

    if (this-> _psensor-> reset () != YAPI::SUCCESS) {
      LOG_ERROR ("Failed to reset energy counting");
      return false;
    }

    this-> _psensor-> set_resolution (0.00001);

    LOG_INFO ("Yocto Watt PDU ", this-> _psensor-> get_friendlyName (), " configured.");
    LOG_INFO ("Power precision[", this-> _psensor-> get_friendlyName (), "] = ", this-> _psensor-> get_resolution (), this-> _psensor-> get_unit ());

    return true;
  }

  void YoctoReader::poll () {
    if (this-> _psensor-> isOnline ()) {
      this-> _currentEnergy = this-> _psensor-> get_meter () * 3600 ;
      this-> _psensor-> reset ();
    }
  }

  double YoctoReader::getEnergy () const {
    return this-> _currentEnergy;
  }

  void YoctoReader::dispose () {
    if (this-> _psensor != nullptr) {
      YAPI::FreeAPI ();
      this-> _psensor = nullptr;
    }
  }

  YoctoReader::~YoctoReader () {
    this-> dispose ();
  }

}
