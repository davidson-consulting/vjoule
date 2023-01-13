#include <iostream>
#include <common/concurrency/proc.hh>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/personality.h>

namespace common::concurrency {

    SubProcess::SubProcess () {}    

    SubProcess::SubProcess (const std::string & cmd, const std::vector <std::string> & args, const std::string & cwd) :
	_cmd (cmd), _args (args), _cwd (cwd)
    {}
	
    
    /**
     * ================================================================================
     * ================================================================================
     * =========================           RUNNING            =========================
     * ================================================================================
     * ================================================================================
     */

    
    void SubProcess::start () {
	this-> _pid = ::fork ();
	if (this-> _pid == 0) {
	    this-> child ();
	}


	this-> _stdin.ipipe ().close ();
	this-> _stdout.opipe ().close ();
	this-> _stderr.opipe ().close ();
    }

    void SubProcess::startDebug () {
	this-> _pid = ::fork ();
	if (this-> _pid == 0) {	    
	    personality(ADDR_NO_RANDOMIZE);
	    ptrace (PTRACE_TRACEME, 0, nullptr, nullptr);
	    this-> child ();
	}

	this-> _stdin.ipipe ().close ();
	this-> _stdout.opipe ().close ();
	this-> _stderr.opipe ().close ();	
    }    

    void SubProcess::child () {
	::dup2 (this-> _stdin.ipipe ().getHandle (), STDIN_FILENO);
	::dup2 (this-> _stdout.opipe ().getHandle (), STDOUT_FILENO);
	::dup2 (this-> _stderr.opipe ().getHandle (), STDERR_FILENO);

	this-> _stdin.close ();
	this-> _stdout.close ();
	this-> _stderr.close ();

	std::vector <const char*> alls;
	alls.push_back (this-> _cmd.c_str ());
	for (auto & it : this-> _args) {
	    alls.push_back (it.c_str ());
	}
	alls.push_back (NULL);

	int n = ::chdir (this-> _cwd.c_str ());

	int out = ::execvp (alls [0],  const_cast<char* const *> (alls.data ()));
	if (out == -1) {
	    printf ("execvp () failed\n");
	}

	exit (0);
    }

    int SubProcess::getPid () const {
	return this-> _pid;
    }

    
    void SubProcess::kill () {
	this-> _stdin.close ();
	::kill (this-> _pid, SIGKILL);	
    }

    int SubProcess::wait () {
	int status = 0;
	this-> _stdin.close ();
	::waitpid (this-> _pid, &status, 0);
	return status;
    }

    /**
     * ================================================================================
     * ================================================================================
     * =========================           GETTERS            =========================
     * ================================================================================
     * ================================================================================
     */

    OPipe & SubProcess::stdin () {
	return this-> _stdin.opipe ();
    }

    IPipe & SubProcess::stdout () {
	return this-> _stdout.ipipe ();
    }

    IPipe & SubProcess::stderr () {
	return this-> _stderr.ipipe ();
    }

    SubProcess::~SubProcess () {
	this-> _stderr.close ();
	this-> _stdout.close ();
	this-> _stdin.close ();
    }
    
}

