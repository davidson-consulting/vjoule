#pragma once

#include <sensor/plugin/_.hh>

namespace sensor::plugin {


    class Factory {
    private :

	// List of loaded plugins
	std::vector <Plugin*> _plugins;

    public:

	/**
	 * Create an empty plugin factory
	 */
	Factory ();
	
    };
    

}
