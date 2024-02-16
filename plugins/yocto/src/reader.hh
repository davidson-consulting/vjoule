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

        // The last meter value read
        double _last;

    public:

        // The sum of the watts read
        double _currentWatts;

        // The number of watt points
        int _nbWatts;

        static YoctoReader __GLOBAL_Yocto__;

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
         * @returns: the average power since the last read
         */
        double getPower () const;

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
