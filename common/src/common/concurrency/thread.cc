#include <common/concurrency/thread.hh>


namespace common {
    
    namespace concurrency {

	thread spawn (void (*func)(thread)) {
	    auto th = new internal::fn_thread_launcher (func);
	    pthread_create (&th-> content, nullptr, &internal::thread_fn_main, th);
	    return th-> content;
	}

	void join (thread th) {
	    pthread_join (th, nullptr);
	}

	void kill (thread th) {
	    pthread_cancel (th);
	}
	
	namespace internal {
	    void* thread_fn_main (void * inner) {
		fn_thread_launcher * fn = (fn_thread_launcher*)(inner);
		fn-> run ();
		delete (char*) inner;
		return nullptr;
	    }

	    void* thread_dg_main (void * inner) {
		dg_thread_launcher * dg = (dg_thread_launcher*)(inner);
		dg-> run ();
		delete (char*) inner;
		return nullptr;
	    }
	    
	    dg_thread_launcher::dg_thread_launcher (fake* closure, void (fake::*func) (thread)) :
		closure (closure),
		func (func),
		content (0)
	    {}

	    void dg_thread_launcher::run () {
		(this-> closure->* (this-> func)) (this-> content);
	    }

	    fn_thread_launcher::fn_thread_launcher (void (*func) (thread)) :
		func (func),
		content (0)
	    {}

	    void fn_thread_launcher::run () {
		this-> func (this-> content);
	    }
	       
	}
	
    }    

}

