#include <iostream>

#include <sys/wait.h>
#include <vjoule.hh>
#include <command.hh>

using namespace tools::vjoule;

int main(int argc, char * argv[]) {
    if (getuid()) {
	std::cerr << "You are not root. This program will only work if run as root." << std::endl;
	exit(-1);
    }
    
    CommandParser parser (argc, argv);

    auto cmd = parser.getCommandLine ();
    if (cmd.type == CommandType::PROFILE) {
	
	
    } else {
	VJoule vjoule(cmd);
	vjoule.run();
    }
    
    return 0;
}
