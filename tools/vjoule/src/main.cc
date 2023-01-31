#include <iostream>

#include <sys/wait.h>
#include <vjoule.hh>
#include <command.hh>
#include <profiler.hh>
#include <top.hh>

using namespace tools::vjoule;

int main(int argc, char * argv[]) {
    if (getuid()) {
	std::cerr << "You are not root. This program will only work if run as root." << std::endl;
	exit(-1);
    }
    
    CommandParser parser (argc, argv);

    auto cmd = parser.getCommandLine ();
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
