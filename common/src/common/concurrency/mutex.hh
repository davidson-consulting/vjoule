#pragma once

#include <pthread.h>

namespace common {

    namespace concurrency {

	class mutex {

	    pthread_mutex_t _m;

	    
	public:
	    
	    mutex ();

	    void lock ();

	    void unlock ();

	    ~mutex ();
	};

	
    }    

}
