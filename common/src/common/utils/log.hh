#pragma once

#include <ctime>
#include <iostream>
#include <sstream>
#include <common/concurrency/mutex.hh>
#include <fstream>

namespace common::utils {

    enum class LogLevel : int {
	NONE = 0,
	ERROR,
	WARN,
	INFO,
	SUCCESS,
	STRANGE,
	DEBUG,
	ALL
    };


#define PURPLE "\e[1;35m"
#define BLUE "\e[1;36m"
#define YELLOW "\e[1;33m"
#define RED "\e[1;31m"
#define GREEN "\e[1;32m"
#define BOLD "\e[1;50m"
#define UNDERLINE "\e[4m"
#define RESET  "\e[0m"

    /**
     * @returns: the current time encoded in string
     */
    std::string get_time ();

    /**
     * @returns: the current time encoded in string (attached with '_' instead of spaces)
     */
    std::string get_time_no_space ();

    /**
     * For variadic call (does nothing)
     */
    void content_print (std::ostream&);

    /**
     * write a in the stream s
     */
    template <typename T>
    void content_print (std::ostream& s, T a) {
	s << a;
    }

    /**
     * write a and b in the stream s
     */
    template <typename T, typename ... R>
    void content_print (std::ostream & s, T a, R... b) {
	s << a;
	content_print (s, b...);
    }
    
    class Logger {
    private :

	// the mutex log is static for multiple logger capability
	static concurrency::mutex __mutex__;

	// The descriptor of the output file
	std::ostream* _stream;

	// The file desc
	std::ofstream _file;

	// The ofstream path
	std::string _path;

	static Logger __globalInstance__;
	
	// The level of logging
	LogLevel _level = LogLevel::SUCCESS;

    private :

	Logger (const Logger & other);

	void operator=(const Logger& other);
	
    public : 

	/**
	 * Create a logger that write to logPath
	 * @params: 
	 *     - logPath: the log file
	 *     - lvl: the level of logging
	 */
	Logger (const std::string & logPath, LogLevel lvl = LogLevel::SUCCESS) ;

	/**
	 * Create a logger that writes to stdout
	 * @params: 
	 *    - lvl: the level of logging
	 */
	Logger (LogLevel lvl = LogLevel::SUCCESS);

	/**
	 * write into logPath from now on
	 * @warning: clear the file at logPath
	 */
	void redirect (const std::string& logPath);

	/**
	 * Change the lvl of the logger
	 */
	void changeLevel (LogLevel lvl);

	/**
	 * change the lvl of the logger from the level name
	 */
	void changeLevel (const std::string & level);
	
	/**
	 * @returns: the path of the log file (might be == "")
	 */
	const std::string & getLogFilePath () const;

	/**
	 * @returns: the global instance of the logger
	 */
	static Logger& globalInstance ();
	
	/**
	 * Info log (LogLevel >= LogLevel::INFO)
	 */
	template <typename ... T>
	void info (T... msg) {
	    if (this-> _level >= LogLevel::INFO) {
		__mutex__.lock ();
		(*this-> _stream) << "[" << BLUE << "INFO" << RESET << "][" << get_time () << "] ";
		content_print (*this-> _stream, msg...);		
		(*this-> _stream) << std::endl;
		__mutex__.unlock ();
	    }
	}

	/**
	 * Debug log (LogLevel >= LogLevel::DEBUG)
	 */
	template <typename ... T>
	void debug (T... msg) {
	    if (this-> _level >= LogLevel::DEBUG) {
		__mutex__.lock ();
		(*this-> _stream) << "[" << PURPLE << "DEBUG" << RESET << "][" << get_time () << "] ";
		content_print (*this-> _stream, msg...);		
		(*this-> _stream) << std::endl;
		__mutex__.unlock ();
	    }
	}

	
	/**
	 * Info log (LogLevel >= LogLevel::ERROR)
	 */
	template <typename ... T>
	void error (T... msg) {
	    if (this-> _level >= LogLevel::ERROR) {
		__mutex__.lock ();
		(*this-> _stream) << "[" << RED << "ERROR" << RESET << "][" << get_time () << "] ";
		content_print (*this-> _stream, msg...);
		(*this-> _stream) << std::endl;
		__mutex__.unlock ();
	    }
	}
       
	/**
	 * Info log (LogLevel >= LogLevel::SUCCESS)
	 */
	template <typename ... T>
	void success (T... msg) {
	    if (this-> _level >= LogLevel::SUCCESS) {
		__mutex__.lock ();
		*this-> _stream << "[" << GREEN << "SUCCESS" << RESET << "][" << get_time () << "] ";
		content_print (*this-> _stream, msg...);
		*this-> _stream << std::endl;
		__mutex__.unlock ();
	    }
	}

	/**
	 * Info log (LogLevel >= LogLevel::WARN)
	 */
	template <typename ... T>
	void warn (T... msg) {
	    if (this-> _level >= LogLevel::WARN) {
		__mutex__.lock ();
		*this-> _stream << "[" << YELLOW << "WARNING" << RESET << "][" << get_time () << "] ";
		content_print (*this-> _stream, msg...);
		*this-> _stream << std::endl;
		__mutex__.unlock ();
	    }
	}
	
	/**
	 * Info log (LogLevel >= LogLevel::STRANGE)
	 */
	template <typename ... T>
	void strange (T... msg) {
	    if (this-> _level >= LogLevel::STRANGE) {
		__mutex__.lock ();
		*this-> _stream << "[" << PURPLE << "STRANGE" << RESET << "][ " << get_time () << "] ";
		content_print (*this-> _stream, msg...);
		*this-> _stream << std::endl;
		__mutex__.unlock ();
	    }
	}

	/**
	 * Dispose the logger (write to stdout from now on)
	 */
	void dispose ();

	/**
	 * Call this-> dispose ()
	 */
	~Logger ();
	
    };
      	
}    
