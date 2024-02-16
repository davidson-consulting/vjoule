#include "reader.hh"

#include <iostream>
#include <stdlib.h>
#include <ctime>


namespace yocto {

  yocto::YoctoReader YoctoReader::__GLOBAL_Yocto__;

  void onPowerEvent (YPower*, const std::string & content) {
    char * end;
    double value = std::strtod (content.c_str (), &end);

    YoctoReader::__GLOBAL_Yocto__._currentWatts += value;
    YoctoReader::__GLOBAL_Yocto__._nbWatts += 1;
  }

  bool YoctoReader::configure (const common::utils::config::dict * config) {
    std::string target = "any";
    if (config != nullptr) {
      target = config-> getOr<std::string> ("target", "any");
    }

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


    // this-> _psensor-> set_reportFrequency ("3/s");
    // this-> _psensor-> set_logFrequency ("3/s");
    this-> _psensor-> reset ();
    //this-> _psensor-> startDataLogger ();
    this-> _psensor-> registerValueCallback (&onPowerEvent);

    LOG_INFO ("Yocto Watt PDU ", this-> _psensor-> get_friendlyName (), " configured.");
    LOG_INFO ("Power precision[", this-> _psensor-> get_friendlyName (), "] = ", this-> _psensor-> get_resolution (), this-> _psensor-> get_unit ());

    this-> _last = this-> _psensor-> get_meter ();
    return true;
  }

  void YoctoReader::poll () {
    if (this-> _psensor-> isOnline ()) {
      this-> _nbWatts = 0;
      this-> _currentWatts = 0;

      std::string errmsg;
      YAPI::HandleEvents (errmsg);

      auto read = this-> _psensor-> get_meter ();
      this-> _currentEnergy = read - this-> _last;
      this-> _last = read;
    }
  }

  double YoctoReader::getEnergy () const {
    return this-> _currentEnergy * 3600;
  }

  double YoctoReader::getPower () const {
    if (this-> _nbWatts == 0) {
      return 0;
    } else {
      return this-> _currentWatts / this-> _nbWatts;
    }
  }

  void YoctoReader::dispose () {
    if (this-> _psensor != nullptr) {
      // this-> _psensor-> stopDataLogger ();
      YAPI::FreeAPI ();
      this-> _psensor = nullptr;
    }
  }

  YoctoReader::~YoctoReader () {
    this-> dispose ();
  }

}
