#pragma once

#include <vector>
#include <string>
#include <chrono>
#include <map>
#include <iostream>
#include <ctime>

namespace vjoule {

    /**
     * Error that can be thrown by the vjoule api
     */
    struct vjoule_error {
        std::string msg;
    };
    
    /**
     * A consumption stamp is the consumption of a given entity (process, machine) at a given instant since the start
     */
    struct consumption_stamp_t {

        // The instant of the consumption
        std::chrono::system_clock::time_point timestamp;

        // The PDU consumption in Joule
        double pdu;

        // The CPU consumption in Joule
        double cpu;

        // The GPU consumption in Joule
        double gpu;

        // The RAM consumption in Joule
        double ram;
	
    };

    /**
     * A consumption diff is the consumption of a given entity between two consumption stamp
     */
    struct consumption_diff_t {

        // The duration in second of the diff
        float duration;

        // The PDU consumption in Joule during the duration
        double pdu;

        // The CPU consumption in Joule during the duration
        double cpu;

        // The GPU consumption in Joule during the duration
        double gpu;

        // The RAM consumption in Joule during the duration
        double ram;
	
    };

    /**
     * A consumption percentage is the percentage of a consumption diff over another
     */
    struct consumption_perc_t {

        // The percentage of duration
        float duration;

        // The percentage of pdu consumption
        float pdu;

        // The percentage of cpu consumption
        float cpu;

        // The percentage of gpu consumption
        float gpu;

        // The percentage of ram consumption
        float ram;

    };


    /**
     * The vjoule api used to create process groups, and retreive energy consumption
     */
    class vjoule_api {
    private: 


        // The inotify handle
        int _inotifFd = 0;

        // The watch handle
        int _inotifFdW = 0;

    private:

        vjoule_api (const vjoule_api & other);

        void operator= (const vjoule_api & other);
	
    public: 
	
        /**
         * Create a vjoule api
         */
        vjoule_api ();

        /**
         * @returns: the consumption of the machine
         */
        consumption_stamp_t get_machine_current_consumption () const;
	
        /**
         * Clean everything started by the api
         */
        ~vjoule_api ();


    private:

        /**
         * Force an iteration of the vjoule service
         */
        void force_sig () const;

        /**
         * Wait for the service to finish computing an iteration
         */
        void wait_iteration () const;

        /**
         * When sig handler is triggered
         */
        void on_exit ();
	
    };
    

}

std::ostream & operator << (std::ostream & o, const vjoule::consumption_stamp_t & c);

std::ostream & operator << (std::ostream & o, const vjoule::consumption_diff_t & c);

std::ostream & operator << (std::ostream & o, const vjoule::consumption_perc_t & c);

vjoule::consumption_diff_t operator- (const vjoule::consumption_stamp_t & left, const vjoule::consumption_stamp_t & right);

vjoule::consumption_diff_t operator+ (const vjoule::consumption_diff_t & left, const vjoule::consumption_diff_t & right);

vjoule::consumption_diff_t operator- (const vjoule::consumption_diff_t & left, const vjoule::consumption_diff_t & right);

vjoule::consumption_perc_t operator% (const vjoule::consumption_diff_t & left, const vjoule::consumption_diff_t & right);
