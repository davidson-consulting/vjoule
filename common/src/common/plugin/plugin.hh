#pragma once

#include <string>
#include <sstream>
#include <common/utils/_.hh>

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>


namespace common::plugin {

    class Plugin {
    private :

	// The path of the plugin (so file)
	std::string _path;

	// The kind of plugin
	std::string _kind;

	// The name of the plugin
	std::string _name;
	
	// The handle to the so file
	void * _handle = nullptr;

	// The dispose function of the plugin
	void (*_handleDispose) () = nullptr;

	// The init function of the plugin
	bool (*_handleInit) (const common::utils::config::dict *) = nullptr;
	
    private:

	Plugin (const Plugin & other);
	
	void operator= (const Plugin & other);
	
    public: 

	/**
	 * @params: 
	 *    - kind: the kind of plugin (cpu, gpu, formula, fpga, ...)
	 */
	Plugin (const std::string & kind, const std::string & name);

	/**
	 * @returns: the path of the plugin
	 */
	const std::string & getPath () const;

	/**
	 * @returns: the name of the plugin
	 */
	const std::string & getName () const;
	
	/**
	 * @returns: true if the plugins is of kind 'kind' and has the name 'name'
	 */
	bool is (const std::string & kind, const std::string & name);

	/**
	 * Configure the plugin so it is now usable
	 * @warning: does not call the init function of the plugin that is made manually by the method 'init'
	 * @returns: true if the logging succeeded, false otherwise
	 */
	bool configure ();
	
	/**
	 * Execute the init function of the plugin
	 */
	bool init (const common::utils::config::dict & config);
	
	/**
	 * @return: a function pointer of the plugin
	 */
	template <typename T>
	T getFunction (const std::string & name) {
	    if (this-> _handle != nullptr) {
		return (T) (dlsym (this-> _handle, name.c_str ()));
	    }

	    return nullptr;
	}

	/**
	 * Execute the dispose function of the plugin
	 * And clear all the handles 
	 */
	void dispose ();

	/**
	 * this-> dispose ();
	 */
	~Plugin ();
	
    };

    
}
