#include <common/utils/log.hh>
#include <iostream>
#include <iomanip>

namespace common::utils {

    concurrency::mutex Logger::__mutex__;

    Logger Logger::__globalInstance__ (LogLevel::SUCCESS);
	
    void content_print () {}
	    
    std::string get_time () {		
	time_t now;
	time(&now);
	char buf[sizeof "2011-10-08 07:07:09"];
	strftime(buf, sizeof buf, "%F %T", gmtime(&now));

	return std::string (buf);
    }

    std::string get_time_no_space () {		
	time_t now;
	time(&now);
	char buf[sizeof "2011-10-08_07:07:09"];
	strftime(buf, sizeof buf, "%F_%T", gmtime(&now));

	return std::string (buf);
    }


    Logger::Logger (LogLevel level) :
	_level (level),
	_stream (&std::cout)
    {	
	(*this-> _stream) << std::fixed << std::setprecision(8);
    }

    Logger::Logger (const std::string& logPath, LogLevel lvl) :
	_level (lvl),
	_stream (&std::cout),
	_file (logPath, std::ios::out)
    {
	this-> _stream = &this-> _file;
	(*this-> _stream) << std::fixed << std::setprecision(8);
	this-> _path = logPath;
    }

    void Logger::redirect (const std::string& logPath) {
	if (logPath != "") {
	    this-> _file = std::ofstream (logPath, std::ios::out);
	    this-> _stream = &this-> _file;
	    this-> _path = logPath;
	} else {
	    this-> _file.close ();
	    this-> _stream = &std::cout;
	}
	
	(*this-> _stream) << std::fixed << std::setprecision(8);
    }

    void Logger::changeLevel (LogLevel lvl) {
	this-> _level = lvl;
    }

    void Logger::changeLevel (const std::string& name) {
	std::string str (name);
	std::transform(str.begin(), str.end(),str.begin(), ::toupper);
	if (str == "NONE") {
	    this-> _level = LogLevel::NONE;
	} else if (str == "ERROR") {
	    this-> _level = LogLevel::ERROR;
	} else if (str == "WARN") {
	    this-> _level = LogLevel::WARN;
	} else if (str == "INFO") {
	    this-> _level = LogLevel::INFO;
	} else if (str == "SUCCESS") {
	    this-> _level = LogLevel::SUCCESS;
	} else if (str == "STRANGE") {
	    this-> _level = LogLevel::STRANGE;
	} else if (str == "DEBUG") {
	    this-> _level = LogLevel::DEBUG;
	} else if (str == "ALL") {
	    this-> _level = LogLevel::ALL;
	} else {
	    this-> _level = LogLevel::SUCCESS;
	}
    }
    
    const std::string& Logger::getLogFilePath () const {
	return this-> _path;
    }

    Logger& Logger::globalInstance () {
	return __globalInstance__;
    }	   

    void Logger::dispose () {
	this-> _file.close ();
	this-> _stream = &std::cout;
	this-> _path = "";
    }	


    Logger::~Logger () {}
}


