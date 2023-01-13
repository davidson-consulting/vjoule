#pragma once

#include <chrono>

namespace common {

    namespace concurrency {

	class timer {
	private : 

	    std::chrono::system_clock::time_point _start_time;

	public :

	    timer ();

	    /**
	     * restart the timer
	     */
	    void reset (std::chrono::system_clock::time_point, std::chrono::duration<double> since) ;

	    /**
	     * restart the timer
	     */
	    void reset () ;

	    /**
	     * @return: the number of seconds since last start
	     */
	    float time_since_start () const;

	    /**
	     * Sleep for nbSec
	     */
	    void sleep (float nbSec);
	    
	};
	
    }

}
