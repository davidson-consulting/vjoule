#pragma once

#include <string>

namespace common::concurrency {

    struct iopipe {
	int i;
	int o;
    };
	
    class OPipe {

	int _pipe;


    private :

	OPipe (const OPipe & other);
	void operator= (const OPipe & other);
	
    public:

	OPipe (int pipe);

	OPipe (OPipe && other);

	void operator=(OPipe && other);
	
	void write (const std::string & msg);	    
	    
	void close ();

	void setNonBlocking ();
	
	int getHandle () const;
	    
	~OPipe ();
	    
    };


    class IPipe {

	int _pipe;

    private :

	IPipe (const IPipe & other);
	void operator= (const IPipe & other);
	
    public: 
       
	IPipe (IPipe && other);

	void operator=(IPipe && other);
	
	IPipe (int pipe);
	
	std::string read ();

	char readC ();
	
	void setNonBlocking ();

	void close ();

	int getHandle () const;

	~IPipe ();
	    
    };

	
    class IOPipe {

	IPipe _ipipe;
	    
	OPipe _opipe;

    private :

	IOPipe (const IOPipe & other);
	void operator= (const IOPipe & other);
	
	    
    public:

	/**
	 * Pipe construction
	 */
	IOPipe ();

	void operator=(IOPipe && other);
	
	IOPipe (iopipe pipes);

	IOPipe (IOPipe && pipes);

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
