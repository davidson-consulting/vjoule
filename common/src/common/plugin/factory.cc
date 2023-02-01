#include <common/plugin/factory.hh>

namespace common::plugin {


    Factory::Factory () {}


    std::vector <Plugin*> Factory::getPlugins (const std::string & kind) const {
	auto it = this-> _sort.find (kind);
	if (it != this-> _sort.end ()) {
	    return it-> second;
	}
	
	return {};
    }

    std::unordered_map <std::string, Plugin*> & Factory::getPlugins () {
	return this-> _plugins;
    }
    
    bool Factory::configurePlugin (const std::string & kind, const common::utils::config::dict & config) {
	if (!config.has <std::string> ("name")) {
	    LOG_ERROR ("Plugin of kind : ", kind, " has no 'name'");
	    return false;
	}

	Plugin * plugin = nullptr;
	bool ret = false;
	
	{
	    auto name = config.get <std::string> ("name");
	    auto it = this-> _plugins.find (name);
	    if (it == this-> _plugins.end ()) {	
		plugin = new Plugin (kind, config.get <std::string> ("name"));
		if (!plugin-> configure ()) {
		    delete plugin;
		    return false;
		}
		
		ret = plugin-> init (config);
		this-> _plugins.emplace (name, plugin);
	    } else {
		plugin = it-> second;
		ret = true;
	    }
	}
	
	auto it = this-> _sort.find (kind);
	if (it == this-> _sort.end ()) {
	    std::vector <Plugin*> v;
	    v.push_back (plugin);
	    this-> _sort.emplace (kind, v);
	} else it-> second.push_back (plugin);

	return ret;
    }

    std::vector <Plugin*> Factory::forget (const std::string & kind) {
	std::vector <Plugin*> ret;
	auto it = this-> _sort.find (kind);
	if (it != this-> _sort.end ()) {
	    ret = std::move (it-> second);
	    for (auto &jt : ret) {
		this-> _plugins.erase (jt-> getName ());
	    }
	    
	    this-> _sort.erase (kind);
	}

	return ret;
    }    

    void Factory::dispose () {
	for (auto & it : this-> _plugins) {
	    it.second-> dispose ();
	    delete it.second;	
	}

	this-> _plugins.clear ();
	this-> _sort.clear ();
    }

    

}
