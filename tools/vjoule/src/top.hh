#pragma once

#include "command.hh"
#include <common/_.hh>

#include "ftxui/component/captured_mouse.hpp"  
#include "ftxui/component/component.hpp"       
#include "ftxui/component/component_base.hpp"  
#include "ftxui/component/screen_interactive.hpp"  
#include "ftxui/dom/elements.hpp" 
#include "ftxui/dom/table.hpp"  
#include "ftxui/screen/color.hpp"  

#include "ftxui/dom/elements.hpp"
#include "ftxui/screen/screen.hpp"
#include "ftxui/screen/string.hpp"


namespace tools::vjoule {

    struct TopError {};
    
    struct Result {
	// Joule
	double cpuJ;
	double ramJ;
	double gpuJ;

	// Watt
	double cpuW;
	double ramW;
	double gpuW;

	// Percentage system in Joule
	double cpuP;
	double ramP;
	double gpuP;
    };
    
    class Top {
    private : 

	// The command line used to configure the profiler
	CommandLine _cmd;

	// The path of the config file
	std::string _cfgPath;

	// The configuration of the running instance of vjoule
	common::utils::config::dict _config;

	// The list of used plugins
	std::map <std::string, std::vector <std::string> > _plugins;

	// The frequency of the service
	int _freq = 1;

	// The header of the top display
	ftxui::Element _summary;

	// The content of consumption
	ftxui::Element _content;

	// The screen rendering the UI
	ftxui::ScreenInteractive _screen = ftxui::ScreenInteractive::Fullscreen();

	// in run loop
	bool _isRunning = false;

	// Screen is ready to receive message
	bool _isReady = false;
	
	// The output file
	FILE * _output = nullptr;
	
    private:

	// Timer used to compute watts
	common::concurrency::timer _timer;

	// Result of the system
	Result _glob;
	
	// The result fetched
	std::map <std::string, Result> _results;

	// The watt history of the system
	std::vector <double> _cpuHist;

	// The watt history of the system
	std::vector <double> _ramHist;

	// The watt history of the system
	std::vector <double> _gpuHist;
	
    public:

	/**
	 * @params: 
	 *    - parameters passed to top command
	 */
	Top (const CommandLine & cmd);

	/**
	 * Run the top command
	 */
	void run ();

    private:


	/**
	 * ===================================================================================
	 * ===================================================================================
	 * ================================   CONFIGURATION     ==============================
	 * ===================================================================================
	 * ===================================================================================
	 */    
	
	/**
	 * Configure the top command
	 */
	void configure ();

	/**
	 * List the plugins in the configuration file
	 */
	void listPlugins ();

	/**
	 * Create the cgroup and attach the pid to it
	 */
	void createAndRegisterCgroup ();

	/**
	 * Dispose everything (delete created cgroup)
	 */
	void dispose ();
	

	/**
	 * ===================================================================================
	 * ===================================================================================
	 * ===================================   RENDERING     ===============================
	 * ===================================================================================
	 * ===================================================================================
	 */    
	
	/**
	 * Update the screen information and trigger a new rendering
	 */
	void display () ;

	/**
	 * Export the results into a the output file
	 */
	void exportCsv ();
	
	/**
	 * Async loop running in a separate thread 
	 * This loop render the screen, and is triggered by display method
	 */
	void asyncDisplay (common::concurrency::thread);

	/**
	 * Create the summary header
	 */
	ftxui::Element createSummary () const;

	/**
	 * @returns: the table for the list of cgroup and consumption
	 */
	ftxui::Element createTable () const;

	/**
	 * @returns: the graph showing the dynamic consumption of the system
	 */
	ftxui::Element createGraph ();

	/**
	 * @returns: the maximum value of the vector
	 */
	double findMax (const std::vector <double> & v) const;

	/**
	 * @returns: the result sorted by cpu percentage
	 */
	std::vector <std::pair <std::string, Result> > sortByCpu () const;
	
	
	/**
	 * ===================================================================================
	 * ===================================================================================
	 * ===================================   RESULT FETCH     ============================
	 * ===================================================================================
	 * ===================================================================================
	 */    
	
	/**
	 * Fetch the result to display
	 */
	void fetch ();
	
	/**
	 * Wait for the next iteration of the service
	 */
	void waitServiceIteration () const;

	/**
	 * Read the consumption recursively
	 */
	void readCgroupTree (std::map <std::string, Result> & result, const std::string & currentPath, float time);

	/**
	 * Read the consumption of a given cgroup
	 */
	void readCgroupConsumption (std::map <std::string, Result> & result, const std::string & currentPath, float time);

	/**
	 * Read the consumption of a component of a cgroup
	 */
	double readConsumption (const std::string & cgroupPath, const std::string & component) const;	
	
	/**
	 * Insert current consumption into history, so it can be displayed with curves
	 */
	void insertHistory ();
    };
    

}
