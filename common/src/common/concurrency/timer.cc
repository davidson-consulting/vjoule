#include <common/concurrency/timer.hh>
#include <unistd.h>
#include <iostream>

namespace common {

    namespace concurrency {

	timer::timer () {
	    this-> reset ();
	}
	
	void timer::reset () {
	    this-> _start_time = std::chrono::system_clock::now ();
	}

	void timer::reset (std::chrono::system_clock::time_point now, std::chrono::duration<double> diff) {
	    this-> _start_time = now;// - std::chrono::duration_cast<std::chrono::duration<long> > (diff);
	}
	
	float timer::time_since_start () const {
	    auto end = std::chrono::system_clock::now();
	    std::chrono::duration<double> diff = end - this-> _start_time;
	    return diff.count ();
	}

	void timer::sleep (float nbSec) {
	    float nbMicros = nbSec * 1000000.0f;
	    usleep ((long) nbMicros);
	}
	
    }

}
