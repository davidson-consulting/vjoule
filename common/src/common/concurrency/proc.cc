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


    SubProcess::SubProcess (SubProcess && other) :
	_stdin (std::move (other._stdin)),
	_stdout (std::move (other._stdout)),
	_stderr (std::move (other._stderr)),
	_cwd (std::move (other._cwd)),
	_args (std::move (other._args)),
	_cmd (std::move (other._cmd)),
	_pid (std::move (other._pid))
    {}

    void SubProcess::operator= (SubProcess && other) {
	this-> _stderr.close ();
	this-> _stdout.close ();
	this-> _stdin.close ();
	
	this-> _stdin = std::move (other._stdin);
	this-> _stdout = std::move (other._stdout);
	this-> _stderr = std::move (other._stderr);
	this-> _cwd = std::move (other._cwd);
	this-> _args = std::move (other._args);
	this-> _cmd = std::move (other._cmd);
	this-> _pid = std::move (other._pid);
    }    
    
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
	
	this-> _stdin.opipe ().setNonBlocking ();
	this-> _stdout.ipipe ().setNonBlocking ();
	this-> _stderr.ipipe ().setNonBlocking ();
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

	this-> _stdin.opipe ().setNonBlocking ();
	this-> _stdout.ipipe ().setNonBlocking ();
	this-> _stderr.ipipe ().setNonBlocking ();
	
    }    

    void SubProcess::child () {
	this-> _stderr.opipe ().setNonBlocking ();
	this-> _stdout.opipe ().setNonBlocking ();
	
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

    bool SubProcess::isFinished () const {
	waitpid (this-> _pid, nullptr, 1);
	if (::kill (this-> _pid, 0) == -1) return true;

	return false;
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

