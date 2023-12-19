#pragma once

#ifndef __PROJECT__
#define __PROJECT__ "YOCTO"
#endif

#define __PLUGIN_VERSION__ "1.3.0"


#include "yocto/yocto_api.h"
#include "yocto/yocto_power.h"
#include <common/utils/_.hh>


namespace yocto {

  class YoctoReader {

    // The yocto sensor
    YPower * _psensor;

    // The current energy value
    double _currentEnergy;

      // double _last;

  public:

    /**
     * Configure the yocto plugin
     */
    bool configure (const common::utils::config::dict *);

    /**
     * Poll energy value
     */
    void poll ();

    /**
     * @returns: the energy measured by of the yocto wattmeter since last time
     */
    double getEnergy () const;

    /**
     * Clear the api
     */
    void dispose ();

    /**
     * this-> dispose ()
     */
    ~YoctoReader ();

  };


}
