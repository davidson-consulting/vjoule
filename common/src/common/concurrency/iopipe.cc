#include <common/concurrency/iopipe.hh>
#include <unistd.h>
#include <sstream>
#include <fcntl.h>

namespace common::concurrency {

    /**
     * ================================================================================
     * ================================================================================
     * =========================            OPIPE             =========================
     * ================================================================================
     * ================================================================================
     */
	
    OPipe::OPipe (int o) : _pipe (o)
    {
	int err_flags = ::fcntl(this-> _pipe, F_GETFL, 0);
	::fcntl(this-> _pipe, F_SETFL, err_flags | O_NONBLOCK);
    }
	
    void OPipe::write (const std::string & msg) {
	int n = ::write (this-> _pipe, msg.c_str (), msg.length ());
    }
	
    void OPipe::close () {	    
	if (this-> _pipe != 0 && this-> _pipe != STDOUT_FILENO && this-> _pipe != STDERR_FILENO) {
	    ::close (this-> _pipe);
	    this-> _pipe = 0;
	}
    }

    int OPipe::getHandle () const {
	return this-> _pipe;
    }
	
    OPipe::~OPipe () {
	this-> close ();
    }

    /**
     * ================================================================================
     * ================================================================================
     * =========================            IPIPE             =========================
     * ================================================================================
     * ================================================================================
     */
	
    IPipe::IPipe (int i) : _pipe (i)
    {
	int err_flags = ::fcntl(this-> _pipe, F_GETFL, 0);
	::fcntl(this-> _pipe, F_SETFL, err_flags | O_NONBLOCK);
    }
	

    std::string IPipe::read () {
	std::ostringstream ss;
	for (;;) {
	    char c = '\0';
	    int n = ::read (this-> _pipe, &c, sizeof (char));
	    if (n != -1 && n != 0) {
		ss << c;
	    } else break;
	}

	return ss.str ();
    }

    void IPipe::close () {
	if (this-> _pipe != 0 && this-> _pipe != STDIN_FILENO) {
	    ::close (this-> _pipe);
	    this-> _pipe = 0;
	}
    }

    int IPipe::getHandle () const {
	return this-> _pipe;
    }
	
    IPipe::~IPipe () {
	this-> close ();
    }

    /**
     * ================================================================================
     * ================================================================================
     * =========================              IO              =========================
     * ================================================================================
     * ================================================================================
     */

	
    iopipe createPipes () {
	iopipe res;
	int pipes[2] = {0, 0};
	int n = ::pipe (pipes);

	res.i = pipes [0];
	res.o = pipes [1];
	    
	return res;
    }
	
    IOPipe::IOPipe () : IOPipe (createPipes ())
    {}
	
    IOPipe::IOPipe (iopipe io) :
	_ipipe (io.i), _opipe (io.o)
    {}

    IPipe & IOPipe::ipipe () {
	return this-> _ipipe;
    }

    OPipe & IOPipe::opipe () {
	return this-> _opipe;
    }

    void IOPipe::close () {
	this-> _ipipe.close ();
	this-> _opipe.close ();
    }
	
    IOPipe::~IOPipe () {
	this-> close ();
    }

}
  
