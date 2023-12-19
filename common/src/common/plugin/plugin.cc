#include <common/plugin/plugin.hh>
#include <common/_.hh>

namespace common::plugin {

    Plugin::Plugin (const std::string & kind, const std::string & name)
        : _path (utils::join_path (utils::join_path (VJOULE_DIR, "plugins"), name) + ".so"),
          _kind (kind),
          _name (name)
    {}

    
    const std::string & Plugin::getPath () const {
        return this-> _path;
    }

    const std::string & Plugin::getName () const {
        return this-> _name;
    }    

    bool Plugin::configure () {
        this-> dispose ();

        this-> _handle = dlopen (this-> _path.c_str (), RTLD_LAZY);
        if (this-> _handle == nullptr) {
            LOG_ERROR ("Failed to load ", this-> _kind, " plugin : ", this-> _path, " ", strerror (errno));
            return false;
        }
        dlerror ();

        this-> _handleInit = (bool (*)(const common::utils::config::dict*)) dlsym (this-> _handle, "init");
        if (this-> _handleInit == nullptr) {
            this-> dispose ();
            LOG_ERROR ("Plugin ", this-> _kind, " in ", this-> _path, " has no 'bool init ()' function, error : ", dlerror ());
            return false;
        }
		
        this-> _handleDispose = (void (*)()) dlsym (this-> _handle, "dispose");
        if (this-> _handleDispose == nullptr) {
            this-> dispose ();
            LOG_ERROR ("Plugin ", this-> _kind, " in ", this-> _path, " has no 'void dispose ()' function, error : ", dlerror ());
            return false;
        }
	
        return true;
    }
    
    bool Plugin::init (const common::utils::config::dict & config) {
        if (this-> _handle != nullptr) {
            return this-> _handleInit (&config);
        } else return false;
    }
    
    void Plugin::dispose () {
        if (this-> _handle != nullptr) {
            this-> _handleDispose ();
            dlclose (this-> _handle);
	    
            this-> _handle = nullptr;
            this-> _handleDispose = nullptr;
            this-> _handleInit = nullptr;
        }
    }

    Plugin::~Plugin () {
        this-> dispose ();
    }
    
    
}
