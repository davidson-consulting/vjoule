#include "command.hh"
#include <iostream>


namespace tools::vjoule {


    CommandParser::CommandParser (int argc, char ** argv) :
	_argc (argc),
	_argv (argv)
    {
	this-> parse ();
    }

    const CommandLine & CommandParser::getCommandLine () const {
	return this-> _content;
    }    

    void CommandParser::parse () {
	if (this-> _argc >= 2) {
	    if (std::string (this-> _argv [1]) == "profile") {
		this-> parseProfile ();
	    } else {
		this-> parseExec (std::string (this-> _argv[1]) == "exec");
	    }	    
	} else {
	    this-> printHelp (CommandType::NONE, false);
	    exit (-1);
	}
    }

    void CommandParser::parseExec (bool withKeyword) {
	uint64_t i = 1;
	if (withKeyword) {
	    i = 2;
	}

	this-> _content = CommandLine {
	    .type = CommandType::EXEC,
	    .verbose = false,
	    .cpu = true,
	    .ram = true,
	    .gpu = true,
	    .rapl = true,
	    .nvidia = true,
	    .subCmd = {}
	};

	bool no_rapl = false, no_nvidia = false;
	for (; i < this-> _argc ; i++) {
	    auto flg = this-> isFlag (this-> _argv[i]);
	    if (flg == HELP_FLAG) {
		this-> printHelp (CommandType::EXEC, true);
		exit (-1);
	    }	    
	    else if (flg == VERSION_FLAG) {
		this-> printVersion ();
		exit (-1);
	    }	    
	    else if (flg == CPU_FLAG) this-> _content.cpu = false;	    
	    else if (flg == RAM_FLAG) this-> _content.ram = false;	    
	    else if (flg == GPU_FLAG) this-> _content.gpu = false;
	    else if (flg == RAPL_FLAG) this-> _content.rapl = false;
	    else if (flg == NVIDIA_FLAG) this-> _content.nvidia = false;
	    else if (flg == OUTPUT_FLAG) {
		this-> _content.output = this-> parseOutput (i);
		i += 1;
	    } else if (flg != "") {
		this-> printHelp (CommandType::EXEC, false);
		this-> printError (CommandType::EXEC, flg);
		exit (-1);
	    } else {
		break;
	    }
	}

	std::vector <std::string> cmd;
	for (; i < this-> _argc ; i++) {
	    cmd.push_back (this-> _argv[i]);
	}
	
	this-> _content.subCmd = std::move (cmd);
	if (this-> _content.subCmd.size () == 0) {
	    this-> printHelp (CommandType::EXEC, false);
	    std::cerr << "Need a subcommand to execute" << std::endl;
	    exit (-1);	    
	}

	this-> checkConsistency ();	
    }

    std::string CommandParser::parseOutput (uint64_t i) const {
	if (i + 1 >= this-> _argc) {
	    this-> printHelp (CommandType::EXEC, false);
	    std::cerr << "Need string parameter for option '--" << OUTPUT_FLAG << "'" << std::endl;
	    exit (-1);
	}

	auto flg = this-> isFlag (this-> _argv[i + 1]);
	if (flg != "") {
	    this-> printHelp (CommandType::EXEC, false);
	    std::cerr << "Need string parameter for option '--" << OUTPUT_FLAG << "'" << std::endl;
	    exit (-1);
	}

	return this-> _argv[i];
    }

    void CommandParser::parseProfile () {
	this-> _content = CommandLine {
	    .type = CommandType::PROFILE,
	    .verbose = false,
	    .cpu = true,
	    .ram = true,
	    .gpu = true,
	    .rapl = true,
	    .nvidia = true,
	    .subCmd = {}
	};

	bool no_nvidia = false, no_rapl = false;
	for (uint64_t i = 2 ; i < this-> _argc ; i++) {
	    auto flg = this-> isFlag (this-> _argv[i]);
	    if (flg == HELP_FLAG) {
		this-> printHelp (CommandType::PROFILE, true);
		exit (-1);
	    }	    
	    else if (flg == VERSION_FLAG) {
		this-> printVersion ();
		exit (-1);
	    }	    
	    else if (flg == CPU_FLAG) this-> _content.cpu = false;	    
	    else if (flg == RAM_FLAG) this-> _content.ram = false;	    
	    else if (flg != "") {
		this-> printHelp (CommandType::PROFILE, false);
		this-> printError (CommandType::PROFILE, flg);
		exit (-1);
	    } else {
		this-> printHelp (CommandType::PROFILE, false);
		this-> printError (CommandType::PROFILE, this-> _argv[i]);
		exit (-1);
	    }
	}

	this-> checkConsistency ();
    }

    void CommandParser::checkConsistency () const {
	if (!this-> _content.rapl && !this-> _content.nvidia) {
	    this-> printHelp (this-> _content.type, false);
	    std::cerr << "No plugin activated, at least rapl or nvidia should be used." << std::endl;
	    exit (-1);
	}

	if (!this-> _content.gpu && !this-> _content.cpu && !this-> _content.ram) {
	    this-> printHelp (this-> _content.type, false);
	    std::cerr << "No device is monitored, at least cpu, ram of gpu should be used." << std::endl;
	    exit (-1);
	}
    }

    void CommandParser::printHelp (CommandType type, bool full) const {
	if (type == CommandType::PROFILE) {
	    std::cout << "./vjoule profile" << std::endl;
	} else if (type == CommandType::EXEC) {
	    std::cout << "./vjoule (exec?) [-nv] [-nr] [--no-cpu] [--no-gpu] [--no-ram] cmd [cmd options...]" << std::endl;
	} else {
	    std::cout << "./vjoule [-h] [-v] (exec | profile)" << std::endl;
	}

	if (full) {
	    if (type == CommandType::PROFILE || type == CommandType::NONE) {
		std::cout << "== subcommand profile : " << std::endl;
		std::cout << "optional arguments : " << std::endl;
		std::cout << "\t-h,--help      \tprint this help and exit" << std::endl;
		std::cout << "\t-v,--version   \tprint version information and exit" << std::endl;
		
		std::cout << "\t    --no-cpu   \tdon't monitor CPU consumption" << std::endl;
		std::cout << "\t    --no-ram   \tdon't monitor RAM consumption" << std::endl;
	    }

	    if (type == CommandType::EXEC || type == CommandType::NONE) {
		std::cout << "== subcommand exec : " << std::endl;
		std::cout << "optional arguments : " << std::endl;
		std::cout << "\t-h,--help             \tprint this help and exit" << std::endl;
		std::cout << "\t-v,--version          \tprint version information and exit" << std::endl;
		std::cout << "\t-nv,--no-nvidia       \tdon't use nvidia plugin for GPU consumption" << std::endl;
		std::cout << "\t-nr,--no-rapl         \tdon't use rapl plugin for CPU, RAM and GPU consumption" << std::endl;
		std::cout << "\t    --no-cpu          \tdon't monitor CPU consumption" << std::endl;
		std::cout << "\t    --no-ram          \tdon't monitor RAM consumption" << std::endl;
		std::cout << "\t    --no-gpu          \tdon't monitor GPU consumption" << std::endl;
		std::cout << "\t-o,--output (file.csv)\tdump the result in a csv file instead of stdout" << std::endl << std::endl;

		std::cout << "positional arguments : " << std::endl;
		std::cout << "\tcmd [options...] \tthe command to run and monitor" << std::endl;

	    }
	}
	
    }

    std::string CommandParser::isFlag (const std::string & flg) const {
	std::string content = "";
	if (flg.size () >= 3 && flg[0] == '-' && flg[1] == '-') {
	    content = flg.substr (2);
	} else if (flg.size () >= 2 && flg[0] == '-') {
	    content = flg.substr (1);
	}

	if (content == VERSION_FLAG || content == VERSION_FLAG_SHORT) return VERSION_FLAG;
	if (content == HELP_FLAG || content == HELP_FLAG_SHORT) return HELP_FLAG;
	if (content == NVIDIA_FLAG || content == NVIDIA_FLAG_SHORT) return NVIDIA_FLAG;
	if (content == RAPL_FLAG || content == RAPL_FLAG_SHORT) return RAPL_FLAG;
	if (content == OUTPUT_FLAG || content == OUTPUT_FLAG_SHORT) return OUTPUT_FLAG;

	return content;
    }
    
    void CommandParser::printVersion () const {}
    
    void CommandParser::printError (CommandType type, const std::string &flg) const {
	std::cerr << "Undefined option '" << flg << "'";
	if (type == CommandType::PROFILE) std::cerr << " for command 'profile'" << std::endl;
	if (type == CommandType::EXEC) std::cerr << " for command 'exec'" << std::endl;
    }
    
    
}
