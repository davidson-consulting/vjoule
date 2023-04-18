#include <common/concurrency/mutex.hh>

namespace common::concurrency {

  mutex::mutex () : _m (PTHREAD_MUTEX_INITIALIZER)
  {}

  void mutex::lock () {
    pthread_mutex_lock (&this-> _m);
  }

  void mutex::unlock () {
    pthread_mutex_unlock (&this-> _m);
  }

  mutex::~mutex () {
    this-> unlock ();
  }

	
}


