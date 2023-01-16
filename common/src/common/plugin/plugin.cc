#include <common/plugin/plugin.hh>
#include <common/_.hh>

namespace common::plugin {

    Plugin::Plugin (const std::string & kind, const std::string & name)
	: _path (utils::join_path (utils::join_path (VJOULE_DIR, "plugins"), name) + ".so"),
	  _kind (kind)
    {}


    Plugin::Plugin (Plugin && other) :
	_path (std::move (other._path)),
	_kind (std::move (other._kind)),
	_handleDispose (other._handleDispose),
	_handlePoll (other._handlePoll),
	_handleInit (other._handleInit),
	_handle (other._handle)
    {
	other._handle = nullptr;
	other._handleDispose = nullptr;
	other._handlePoll = nullptr;
	other._handleInit = nullptr;
    }

    void Plugin::operator= (Plugin && other) {
	this-> dispose ();
	this-> _path = std::move (other._path);
	this-> _kind = std::move (other._kind);

	this-> _handleInit = other._handleInit;
	this-> _handlePoll = other._handlePoll;
	this-> _handleDispose = other._handleDispose;
	this-> _handle = other._handle;

	other._handle = nullptr;
	other._handleDispose = nullptr;
	other._handlePoll = nullptr;
	other._handleInit = nullptr;
    }
    
    const std::string & Plugin::getPath () const {
	return this-> _path;
    }

    const std::string & Plugin::getName () const {
	return this-> _name;
    }
    
    bool Plugin::init (const common::utils::config::dict & config) {
	this-> dispose ();

	this-> _handle = dlopen (this-> _path.c_str (), RTLD_LAZY);
	if (this-> _handle == nullptr) {
	    utils::Logger::globalInstance ().error ("Failed to load ", this-> _kind, " plugin : ", this-> _path);
	    return false;
	}
	dlerror ();

	this-> _handleInit = (bool (*)(const common::utils::config::dict*)) dlsym (this-> _handle, "init");
	if (this-> _handleInit == nullptr) {
	    this-> dispose ();
	    utils::Logger::globalInstance ().error ("Plugin ", this-> _kind, " in ", this-> _path, " has no 'bool init ()' function, error : ", dlerror ());
	    return false;
	}
	

	this-> _handlePoll = (bool (*)()) dlsym (this-> _handle, "poll");
	if (this-> _handlePoll == nullptr) {
	    this-> dispose ();
	    utils::Logger::globalInstance ().error ("Plugin ", this-> _kind, " in ", this-> _path, " has no 'bool poll ()' function, error : ", dlerror ());
	    return false;
	}

	this-> _handleDump = (void (*)(std::ostream&)) dlsym (this-> _handle, "dump");
	if (this-> _handleDump == nullptr) {
	    this-> dispose ();
	    utils::Logger::globalInstance ().error ("Plugin ", this-> _kind, " in ", this-> _path, " has no 'void dump (std::stringstream&)' function, error : ", dlerror ());
	    return false;
	}

	
	this-> _handleDispose = (void (*)()) dlsym (this-> _handle, "dispose");
	if (this-> _handleDispose == nullptr) {
	    this-> dispose ();
	    utils::Logger::globalInstance ().error ("Plugin ", this-> _kind, " in ", this-> _path, " has no 'void dispose ()' function, error : ", dlerror ());
	    return false;
	}
	
	return this-> _handleInit (&config);
    }

    bool Plugin::poll () {
	if (this-> _handlePoll == nullptr) {
	    utils::Logger::globalInstance ().error ("Using invalid plugin (maybe just not init) ", this-> _kind, " at : ", this-> _path, ", error : no 'bool poll ()' function");
	    return false;
	}

	return this-> _handlePoll ();
    }

    void Plugin::dump (std::ostream & s) {
	if (this-> _handleDump == nullptr) {
	    utils::Logger::globalInstance ().warn ("Using invalid plugin (maybe just not init) ", this-> _kind, " at : ", this-> _path, ", error : no 'void dump (std::stringstream&)' function");
	    return;
	}

	this-> _handleDump (s);
    }
    
    void Plugin::dispose () {
	if (this-> _handle != nullptr) {
	    this-> _handleDispose ();
	    dlclose (this-> _handle);
	    
	    this-> _handle = nullptr;
	    this-> _handleDispose = nullptr;
	}
    }

    Plugin::~Plugin () {
	this-> dispose ();
    }
    
    
}
