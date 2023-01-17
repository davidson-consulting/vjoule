#pragma once

#define __PROJECT__ "SENSOR"

#include <common/_.hh>
#include <common/foreign/CL11.hpp>

namespace sensor {
    
    class Sensor {
    private:
	
	// The path of the configuration file
	std::string _cfgPath;

	// The time to wait between to dumping/core
	float _freq;
	
    private :
	
	// The factory of plugins
	common::plugin::Factory _factory;

    private: 
	
	// The core plugin
	common::plugin::Plugin* _core;
	
	common::plugin::CoreComputeFunc_t _computeCore = nullptr;
	
    private:

	// The number of parameters passed to the program
	int _argc;

	// The parameters passed to the program
	char ** _argv;

	// The app option parser
	CLI::App _app;

	CLI::Option * _cfgPathOpt;
	
	CLI::Option * _logPathOpt;

	CLI::Option * _logLvlOpt;

	CLI::Option * _freqOpt;
	
    public:

	/**
	 * @params: 
	 *    - argc: command line option size
	 *    - argv: command line option content
	 */
	Sensor (int argc, char ** argv);
	
	/**
	 * Run the sensor (infinite loop)
	 */
	void run () ;
	
    private :

	/**
	 * Init the option of the app (command line)
	 */
	void initAppOptions ();

	/**
	 * Configure the sensor from a configuration file
	 */
	void configure (const common::utils::config::dict & config);	

	/**
	 * Configure the core of the sensor
	 */
	void configureCore (const common::utils::config::dict & config);

	/**
	 * Configure the options of the command line
	 */
	void configureOptions ();
	
	/**
	 * Configure the plugin with a given configuration
	 */
	void configurePlugin (const std::string & kind, const common::utils::config::dict & config);
	
	/**
	 * Dispose all loaded plugins/core of the sensor
	 */
	void dispose ();
	
    };
    
}
