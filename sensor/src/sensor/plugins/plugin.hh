#pragma once

#include <string>
#include <sstream>

namespace sensor::plugin {

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

	// The poll function of the plugin
	bool (*_handlePoll) () = nullptr;

	// The dumping function of the plugin
	void (*_handleDump) (std::stringstream &) = nullptr;

	// The init function of the plugin
	bool (*_handleInit) () = nullptr;
	
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
	 * Move ctor
	 */
	Plugin (Plugin && other);

	/**
	 * Move affect
	 */
	void operator= (Plugin && other);

	/**
	 * @returns: true if the plugins is of kind 'kind' and has the name 'name'
	 */
	bool is (const std::string & kind, const std::string & name);
	
	/**
	 * Execute the init function of the plugin
	 */
	bool init ();
	
	/**
	 * Execute the poll function of the plugin
	 */
	bool poll ();
	
	/**
	 * Dump the plugin informations into stream
	 */
	void dump (std::stringstream & stream);

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
