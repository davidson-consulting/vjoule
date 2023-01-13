#pragma once

#include <string>

namespace common::concurrency {

    struct iopipe {
	int i;
	int o;
    };
	
    class OPipe {

	int _pipe;

    public:

	OPipe (int pipe);

	void write (const std::string & msg);	    
	    
	void close ();

	int getHandle () const;
	    
	~OPipe ();
	    
    };


    class IPipe {

	int _pipe;

    public:

	IPipe (int pipe);

	std::string read ();
	    
	void close ();

	int getHandle () const;

	~IPipe ();
	    
    };

	
    class IOPipe {

	IPipe _ipipe;
	    
	OPipe _opipe;

	    
    public:

	/**
	 * Pipe construction
	 */
	IOPipe ();

	IOPipe (iopipe pipes);

	/**
	 * @returns: the reading pipe 
	 */
	IPipe & ipipe ();

	/**
	 * @returns: the writting pipe
	 */
	OPipe & opipe ();

	/**
	 * Close both pipes
	 */
	void close ();
	    
	/**
	 * close inner pipes
	 */
	~IOPipe ();
	    
    };
	
}   
