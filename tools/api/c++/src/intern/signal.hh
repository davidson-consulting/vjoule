#pragma once

#include "functional.hh"
#include <list>

namespace common::concurrency {

    /**
     * A signal connect events to function pointers or delegates
     * The slots are called in the order of the connections
     * The signal does not travers threads, so the slots are executed in the thread that is emitting
     * @memory: allocate a func_ptr or delegate for each connection, NO_GC
     */
    template <typename ... T> 
    class signal {
    private: 

		/// List of connections
		std::list <utils::func_ptr <void, T...> *> _connects;

    public :

	    
		signal () {}
	    
		/**
		 * Deep copy of other
		 */
		signal (const signal<T...> & other) {
			for (auto & it : other._connects) {
				this-> _connects.push_back (it-> clone ());
			}
		}


		/**
		 * Deep copy of other
		 */
		const signal<T...>& operator= (const signal<T...> & other) {
			for (auto & it : this-> _connects) {
				delete it;
			}
		
			for (auto & it : other._connects) {
				this-> _connects.push_back (it-> clone ());
			}
		
			return other;
		}

	    
		/**
		 * Connect the signal to a new slot
		 * @params:
		 *    - func: the function slot that will be called at `emit`
		 * @complexity: O(1)
		 */
		void connect (void (*func) (T...)) {
			this-> _connects.push_back (new utils::func_ptr <void, T...> (func));
		}
	    
		/**
		 * Connect the signal to a new slot
		 * @params:
		 *    - x: the closure of the delegate function
		 *    - func: the function slot that will be called at `emit`
		 * @complexity: O(1)
		 */
		template <class X>
		void connect (X * x, void(X::*func) (T...)) {
			this-> _connects.push_back (new utils::delegate <X, void, T...> (x, func));
		}

	    
		/**
		 * Connect the signal to a new slot
		 * @params:
		 *    - x: the closure of the delegate function
		 *    - func: the function slot that will be called at `emit`
		 * @complexity: O(1)
		 */
		template <class X>
		void connect (const X * x, void(X::*func) (T...) const) {
			this-> _connects.push_back (new utils::const_delegate <X, void, T...> (x, func));
		}

	    
		/**
		 * Disconnect the function slots
		 * @info: if the slots is not found, does not do anything
		 * @complexity: O(n)
		 */
		void disconnect (void (* func)(T...)) {
			auto ptr = utils::func_ptr<void, T...> (func);
			for (auto it = this-> _connects.begin (); it != this-> _connects.end (); it ++) {
				if (ptr == **(it)) {
					delete *it;
					it = this-> _connects.erase (it);
				}
			}
		}

		/**
		 * Disconnect the function slots
		 * @info: if the slots is not found, does not do anything
		 * @warning: the closure must be the same as the one used to connect, Cf. functional => common::utils::delegate
		 * @complexity: O(n)
		 */
		template <class X>
		void disconnect (X* x, void (X::*func) (T...)) {
			auto ptr = utils::delegate <X, void, T...> (x, func);
			for (auto it = this-> _connects.begin () ; it != this-> _connects.end (); it ++) {
				if (ptr == **it) {
					delete *it;
					it = this-> _connects.erase (it);
				}
			}
		}
	    
		/**
		 * Disconnect the function slots
		 * @info: if the slots is not found, does not do anything
		 * @warning: the closure must be the same as the one used to connect, Cf. functional => common::utils::delegate
		 * @complexity: O(n)
		 */
		template <class X>
		void disconnect (const X* x, void (X::*func) (T...) const) {
			auto ptr = utils::delegate <X, void, T...> (x, func);
			for (auto it = this-> _connects.begin () ; it != this-> _connects.end (); it ++) {
				if (ptr == **it) {
					delete *it;
					it = this-> _connects.erase (it);
				}
			}
		}
	    
		/**
		 * Emit the signal and call the slots
		 * @complexity: O(n)
		 */
		void emit (T... args) const {
			for (auto it = this-> _connects.begin (); it != this-> _connects.end () ; it++) {
				(**it) (args...);
			}
		}

		void dispose () {
			for (auto it : this-> _connects) {
				delete it;
			}
			this-> _connects.clear ();
		}
      
		~signal () {
			this-> dispose ();
		}
	    
    };
	
}
    

