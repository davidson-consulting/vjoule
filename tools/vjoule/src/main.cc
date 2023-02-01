#include <iostream>

#include <sys/wait.h>
#include <vjoule.hh>
#include <command.hh>
#include <profiler.hh>
#include <top.hh>

using namespace tools::vjoule;


void signal_callback_handler(int signum) {
    
}

int main(int argc, char * argv[]) {    
    CommandParser parser (argc, argv);
    auto cmd = parser.getCommandLine ();

    if (getuid () != 0) {
	common::utils::authenticateSudo ();
    }
    
    if (cmd.type == CommandType::PROFILE) {
	Profiler prf (cmd);
	prf.run ();
    } else if (cmd.type == CommandType::TOP) {
	Top t (cmd);
	t.run ();
    } else {
	VJoule vjoule(cmd);
	vjoule.run();
    }
    
    return 0;
}
