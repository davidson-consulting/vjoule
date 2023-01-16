#pragma once

#include <common/plugin/plugin.hh>
#include <unordered_map>
#include <string>
#include <vector>

namespace common::plugin {


    class Factory {
    private :

	// List of loaded sorted by category
	std::unordered_map <std::string, std::vector <Plugin*> > _sort;

	// List of loaded plugins
	std::unordered_map <std::string, Plugin*> _plugins;

    public:

	/**
	 * Create an empty plugin factory
	 */
	Factory ();

	/**
	 * @returns: the list of plugin of a given kind (e.g. 'formula', 'cpu', ...)
	 */
	std::vector <Plugin*> getPlugins (const std::string & kind) const;

	/**
	 * @returns: all loaded plugins
	 */
	std::unordered_map<std::string, Plugin*> & getPlugins ();
	
	/**
	 * Configure a plugin with a configuration
	 * @params: 
	 *    - kind: the kind of plugin to load
	 *    - config: the configuration to pass to the plugin
	 */
	bool configurePlugin (const std::string & kind, const common::utils::config::dict & config);

	/**
	 * Forget all the plugins of a given kind
	 * @params: 
	 *   - kind: the kind of plugin to forget
	 * @warning: 
	 * this does not dispose the plugins, but remove any reference the factory might have about them
	 * So it is up to the code that called this function to dispose and free the plugins returned by the function
	 */
	std::vector <Plugin*> forget (const std::string & kind);
	

	/**
	 * Dispose all loaded plugins
	 */
	void dispose ();
	
    };
    

}
