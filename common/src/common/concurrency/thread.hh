#pragma once
#include <pthread.h>
#include <tuple>

namespace common {

    namespace concurrency {

	typedef pthread_t thread;
	
	namespace internal {
	    void* thread_fn_main (void * inner);
	    void* thread_dg_main (void * inner);

	    class fake {};

	    
	    class dg_thread_launcher {
	    public:
		thread content;
		fake* closure;
		void (fake::*func) (thread);

		dg_thread_launcher (fake* closure, void (fake::*func) (thread));

		virtual void run ();
	    };

	    class fn_thread_launcher {
	    public:
		thread content;
		void (*func) (thread);

		fn_thread_launcher (void (*func) (thread));

		virtual void run ();
	    };
	    
	    template <typename ... T>
	    class dg_thread_launcher_template : public dg_thread_launcher {
	    public:
		thread content;
		fake * closure;
		std::tuple <T...> datas;
		void (fake::*func) (thread, T...);

		dg_thread_launcher_template (fake* closure, void (fake::*func) (thread, T...), T... args) :
		    dg_thread_launcher (nullptr, nullptr),
		    content (0), closure (closure), func (func), datas (std::make_tuple (args...)) {}

		void run () override  {
		    std::apply ([this](auto &&... args) {
				    (this-> closure->* (this-> func)) (this-> content, args...);
				}, this-> datas);
		}
	    };

	    template <typename ... T>
	    class fn_thread_launcher_template : public fn_thread_launcher {
	    public: 
		thread content;
		void (*func) (thread, T...);
		std::tuple <T...> datas;
		
		fn_thread_launcher_template (void (*func) (thread, T...), T... args) :
		    fn_thread_launcher (nullptr),
		    content (0), func (func), datas (std::make_tuple (args...))
		    {}

		void run () override {
		    std::apply (this-> func, std::tuple_cat (std::make_tuple (this-> content), this-> datas));
		}
		
	    };
	}

	/**
	 * Spawn a new thread that will run a function
	 * @params: 
	 *  - func: the main function of the thread
	 */
	thread spawn (void (*func) (thread));

	template <typename ... T>
	thread spawn (void (*func) (thread, T...), T... args) {
	    auto th = new internal::fn_thread_launcher_template<T...> (func, args...);
	    pthread_create (&th-> content, nullptr, &internal::thread_fn_main, th);
	    return th-> content;
	}
	
	/**
	 * Spawn a new thread that will run a method
	 * @params: 
	 *  - func: the main method of the thread
	 */
	template <class X>
	thread spawn (X * x, void (X::*func)(thread)) {
	    auto th = new internal::dg_thread_launcher ((internal::fake*) x, (void (internal::fake::*)(thread)) func);
	    pthread_create (&th-> content, nullptr, &internal::thread_dg_main, th);
	    return th-> content;
	}


	template <class X, typename ... T>
	thread spawn (X * x, void (X::*func)(thread, T...), T... args) {
	    auto th = new internal::dg_thread_launcher_template ((internal::fake*)x, (void (internal::fake::*)(thread, T...)) func, args...);
	    pthread_create (&th-> content, nullptr, &internal::thread_dg_main, th);
	    return th-> content;
	}

	/**
	 * Wait the end of the execution of the thread
	 * @params: 
	 *  - th: the thread to wait
	 */
	void join (thread th);

	/**
	 * Kill a running thread
	 * @params: 
	 *  - th: the thread to kill
	 */
	void kill (thread th);
	
    }

}
