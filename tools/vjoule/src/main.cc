#include <iostream>

#include <vjoule_api.hh>
#include <sys/wait.h>
#include <exec.hh>
#include <command.hh>
#include <top.hh>

using namespace tools::vjoule;

namespace tools::vjoule {
    common::concurrency::signal<> exitSignal;    
}

void ctrlCHandler (int signum) {
    exitSignal.emit ();
    
    exit (-1);
}

int main(int argc, char * argv[]) {

    signal(SIGINT, &ctrlCHandler);
    
    try {
        CommandParser parser (argc, argv);
        auto cmd = parser.getCommandLine ();

        if (cmd.type == CommandType::TOP) {
            Top t (cmd);
            t.run ();
        } else {
            Exec e(cmd);
            e.run();
        }

    } catch (const CommandError & error) {
        return -1;
    } catch (const TopError & error) {
        return -1;
    } catch (const ::vjoule::vjoule_error & error) {
        std::cerr << error.msg << std::endl;
        return -1;
    }


    
    return 0;
}
