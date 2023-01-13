#pragma once

#include <string>
#include <vector>
#include <set>
#include <common/concurrency/_.hh>
#include <common/net/_.hh>
#include <common/foreign/CL11.hpp>
#include <common/utils/_.hh>

namespace common::sensor {


    class Sensor {
    private:
	
	/**
	 * ===================================================================================
	 * ================================   MAIN LOOP FIELDS    ============================
	 * ===================================================================================
	 */    
	
	// The mutex to sync the sensor main loop thread
	common::concurrency::mutex _mainLoopMutex;

	// The thread of the sensor loop
	common::concurrency::thread _mainLoop;
	
	// true if the main loop thread is running
	bool _mainLoopRunning;

	// The time to wait between to frames
	float _freq;
	
	/**
	 * ===================================================================================
	 * ================================   CLIENT LOOP FIELDS    ==========================
	 * ===================================================================================
	 */    
	
	// The connected formulas
	std::vector <common::net::TcpStream> _clients;

	// The cache to avoid reallocate at each frame
	std::vector <char> _cache;
	
	// The mutex to sync the client thread
	common::concurrency::mutex _clientMutex;

	// The thread of the accept loop for incoming connections
	common::concurrency::thread _acceptLoop;
	
	// The listening address
	common::net::SockAddrV4 _listeningAddr;
	
	// true if the accept loop thread is running
	bool _acceptClients;

	// The listener accepting connections
	common::net::TcpListener _listener;

    protected: 
	
	// The head
	common::net::Header _head;

	// The list of cgroup events to read
	std::vector <std::string> _cgroupEvents;

	// The list of rapl events to read
	std::vector <std::string> _systemEvents;

	// true if we need to sleep between main loop iterations (for nfs reader for examples)
	bool _sleepActive;

	// The path of the configuration file
	std::string _cfgPath;
	
    private : 

	// The number of parameters passed to the program
	int _argc;

	// The parameters passed to the program
	char ** _argv;

	// The app option parser
	CLI::App _app;
	
	CLI::Option * _cfgPathOpt;

	CLI::Option * _logPathOpt;

	CLI::Option * _logLvlOpt;

	CLI::Option * _portOpt;

	CLI::Option * _freqOpt;

	
    protected :

	/**
	 * @params: 
	 *    - name: the name of the sensor (just for debug purposes)
	 *    - argc: the number of param passed to the program
	 *    - argv: the list of param param to the program
	 */
	Sensor (const std::string & name, int argc, char ** argv);
	
    public :

	/**
	 * Start the sensor
	 * @warning: infinite loop
	 */
	virtual void run () final;
	

    protected :

	/**
	 * ===================================================================================
	 * ===================================================================================
	 * ================================   TO IMPLEMENT    ================================
	 * ===================================================================================
	 * ===================================================================================
	 */    
	
	/**
	 * Function called before the run is started
	 */
	virtual void preRun ();

	/**
	 * Function called when run thread are started
	 * @warning: this function should wait for the end of the thread otherwise the sensor will quit immediately
	 * @info: no need to call dispose, it is called by the Sensor::run function
	 */
	virtual void join () = 0;

	/**
	 * Function called at the end of the configuration function
	 * @params: 
	 *    - cfg: the configuration read in the config file or generated from argument line
	 */
	virtual void postConfigure (const common::utils::config::dict & cfg);

	/**
	 * Function called in the main loop each iteration
	 */
	virtual void onUpdate () = 0;

	/**
	 * Function called to clean everything opened in the sensor
	 */
	virtual void dispose () = 0;
	
	/**
	 * ===================================================================================
	 * ===================================================================================
	 * ======================================   CLIENTS    ===============================
	 * ===================================================================================
	 * ===================================================================================
	 */    
	
	/**
	 * Send a packet to the connected clients
	 */
	virtual void sendPacketToClients (const common::net::Packet & pack) final;
      	
	/**
	 * Start the accepting loop
	 */
	virtual void startListener () final;
	
	/**
	 * Close the accepting loop
	 */
	virtual void closeListener () final;

	/**
	 * Start the main loop of the sensor
	 */
	virtual void startMainLoop () final;

	/**
	 * Close the main loop of the sensor
	 */
	virtual void closeMainLoop () final;

	/**
	 * Join the main loop of the sensor
	 */
	virtual void joinMainLoop () final;
	
	/**
	 * Reconfigure the sensor (after a context change for example)
	 */
	virtual void reconfigure () final;
	
    protected :

	/**
	 * Init the option parser and parse the args
	 */
	void initAppOptions ();

	/**
	 * Function used to add custom options to the app in child sensors
	 */
	virtual void addCustomOptions (CLI::App & app);
	
	/**
	 * Configure the sensor from the configuration file
	 */
	void configure ();	

	/**
	 * Configure the sensor from the program options
	 */
	void configureOptions ();
	
	/**
	 * The main loop thread
	 */
	void mainLoop (common::concurrency::thread);

	/**
	 * The loop waiting for incoming connections
	 */
	void acceptLoop (common::concurrency::thread);	

	/**
	 * Write the port of the listener to the port file of the sensor
	 */
	void writePortFile (unsigned short port);

	/**
	 * ===================================================================================
	 * ===================================================================================
	 * ======================================   UTILS    =================================
	 * ===================================================================================
	 * ===================================================================================
	 */    
	
	/**
	 * Transform a config array into a vector
	 */
	std::vector <std::string> getEvents (const common::utils::config::array& events, const std::set<std::string> & ignore) const;

	/**
	 * Create an header from a list of events
	 */
	net::Header createHeader (const std::vector <std::string> & systemEvents, const std::vector <std::string> & cgroupEvents) const;
    };

    
}
