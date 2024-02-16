#include <sys/wait.h>
#include <fstream>

#include <watcher.hh>
#include <exec.hh>
#include <exiting.hh>

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <iomanip>


using namespace common;

namespace tools::vjoule {

    Exec::Exec(const CommandLine & cmd) :
		_cmd (cmd)
    {
		std::vector<std::string> subargs;
		if (this-> _cmd.subCmd.size () >= 2) {
			subargs.insert (subargs.end (), this-> _cmd.subCmd.begin () + 1, this-> _cmd.subCmd.end ());
		}

		this->_child = concurrency::SubProcess(this-> _cmd.subCmd [0], subargs, ".");
    }

    void Exec::dispose () {
		if (this-> _child.getPid () != 0) {
			this-> _child.kill ();
		}
    }

    void Exec::run() {
		exitSignal.connect (this, &Exec::dispose);

		auto start = this-> _api.get_machine_current_consumption ();

		this-> _child.start (false);
		this-> _child.wait ();

		auto end  = this-> _api.get_machine_current_consumption ();
		this-> dispose ();

		auto diff = end - start;

		std::cout << std::endl << std::endl;
		std::cout << "time\t" << common::utils::duration_format (diff.duration) << std::endl;
		if (diff.pdu > 10000) {
			std::cout << "PDU\t" <<	std::fixed << std::setprecision(2) << diff.pdu / 1000 << " kJ" << std::endl;
		} else {
			std::cout << "PDU\t" << std::fixed << std::setprecision(2) << diff.pdu << " J" << std::endl;
		}

		if (diff.cpu > 10000) {
			std::cout << "CPU\t" <<	std::fixed << std::setprecision(2) << diff.cpu / 1000 << " kJ" << std::endl;
		} else {
			std::cout << "CPU\t" << std::fixed << std::setprecision(2) << diff.cpu << " J" << std::endl;
		}

		if (diff.ram > 10000) {
			std::cout << "RAM\t" <<	std::fixed << std::setprecision(2) << diff.ram / 1000 << " kJ" << std::endl;
		} else {
			std::cout << "RAM\t" << std::fixed << std::setprecision(2) << diff.ram << " J" << std::endl;
		}

		if (diff.gpu > 10000) {
			std::cout << "GPU\t" <<	std::fixed << std::setprecision(2) << diff.gpu / 1000 << " kJ" << std::endl;
		} else {
			std::cout << "GPU\t" << std::fixed << std::setprecision(2) << diff.gpu << " J" << std::endl;
		}
    }

    void Exec::runParent (uint64_t childPid, int pipe) {

    }


}
