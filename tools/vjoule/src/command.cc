#include "command.hh"
#include <iostream>

#include <common/_.hh>

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
			if (std::string (this-> _argv [1]) == "top") {
				this-> parseTop ();
			} else {
				this-> parseExec (std::string (this-> _argv[1]) == "exec");
			}
		} else {
			this-> printHelp (CommandType::NONE, false);
			throw CommandError ();
		}
    }

    /**
     * ===================================================================================
     * ===================================================================================
     * ================================   SUB COMMANDS     ===============================
     * ===================================================================================
     * ===================================================================================
     */    


    void CommandParser::parseExec (bool withKeyword) {
		uint64_t i = 1;
		if (withKeyword) {
			i = 2;
		}

		this-> _content = CommandLine {
			.type = CommandType::EXEC,
			.output = "",
			.subCmd = {}
		};

		for (; i < this-> _argc ; i++) {
			auto flg = this-> isFlag (this-> _argv[i]);
			if (flg == HELP_FLAG) {
				this-> printHelp (withKeyword ? CommandType::EXEC : CommandType::NONE, true);
				throw CommandError ();
			}
			else if (flg == VERSION_FLAG) {
				this-> printVersion ();
				throw CommandError ();
			}
			else if (flg != "") {
				this-> printHelp (CommandType::EXEC, false);
				this-> printError (CommandType::EXEC, flg);
				throw CommandError ();
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
			throw CommandError ();
		}
    }

    void CommandParser::parseTop () {
		this-> _content = CommandLine {
			.type = CommandType::TOP,
			.output = "",
			.subCmd = {}
		};

		for (uint64_t i = 2 ; i < this-> _argc ; i++) {
			auto flg = this-> isFlag (this-> _argv[i]);
			if (flg == HELP_FLAG) {
				this-> printHelp (CommandType::TOP, true);
				throw CommandError ();
			}
			else if (flg == VERSION_FLAG) {
				this-> printVersion ();
				throw CommandError ();
			}
			else if (flg == OUTPUT_FLAG) {
				i += this-> parseOutput (i, this-> _content.output);
			}
			else if (flg != "") {
				this-> printHelp (CommandType::TOP, false);
				this-> printError (CommandType::TOP, flg);
				throw CommandError ();
			} else {
				this-> printHelp (CommandType::TOP, false);
				this-> printError (CommandType::TOP, this-> _argv[i]);
				throw CommandError ();
			}
		}
    }

    /**
     * ===================================================================================
     * ===================================================================================
     * ===================================  VALIDATION  ==================================
     * ===================================================================================
     * ===================================================================================
     */

	uint64_t CommandParser::parseOutput (uint64_t i, std::string & res) const {
		if (i + 1 >= this-> _argc) {
			this-> printHelp (CommandType::EXEC, false);
			std::cerr << "Need string parameter for option '--" << OUTPUT_FLAG << "'" << std::endl;
			throw CommandError ();
		}

		auto flg = this-> isFlag (this-> _argv[i + 1]);
		if (flg != "") {
			this-> printHelp (CommandType::EXEC, false);
			std::cerr << "Need string parameter for option '--" << OUTPUT_FLAG << "'" << std::endl;
			throw CommandError ();
		}

		res = this-> _argv[i+1];
		return 1;
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

		return content;
    }    

    /**
     * ===================================================================================
     * ===================================================================================
     * ===================================   PRINTING   ==================================
     * ===================================================================================
     * ===================================================================================
     */    
   
    
    void CommandParser::printHelp (CommandType type, bool full) const {
		if (type == CommandType::EXEC) {
			std::cout << "./vjoule (exec?) cmd [cmd options...]" << std::endl;
		} else if (type == CommandType::TOP) {
			std::cout << "./vjoule top [--output file]" << std::endl;
		} else {
			std::cout << "./vjoule [-h] [-v] (exec | top)" << std::endl;
		}

		if (full) {
			if (type == CommandType::EXEC || type == CommandType::NONE) {
				std::cout << "== subcommand exec : " << std::endl;
				std::cout << "optional arguments : " << std::endl;
				std::cout << "\t-h,--help               \tprint this help and exit" << std::endl;
				std::cout << "\t-v,--version            \tprint version information and exit" << std::endl << std::endl;

				std::cout << "positional arguments : " << std::endl;
				std::cout << "\tcmd [options...]        \tthe command to run and monitor" << std::endl << std::endl;
			}

			if (type == CommandType::TOP || type == CommandType::NONE) {
				std::cout << "== subcommand top : " << std::endl;
				std::cout << "optional arguments : " << std::endl;
				std::cout << "\t-h,--help               \tprint this help and exit" << std::endl;
				std::cout << "\t-v,--version            \tprint version information and exit" << std::endl << std::endl;
				std::cout << "\t-o,--output (file.csv)  \tdump the result in a csv file instead of stdout" << std::endl << std::endl;
			}
		}
    }
    
    void CommandParser::printVersion () const {
		std::cout << "vjoule (" << __VJOULE_VERSION__ << ")" << std::endl;
		std::cout << __COPYRIGHT__ << std::endl << std::endl;
    }
    
    void CommandParser::printError (CommandType type, const std::string &flg) const {
		std::cerr << "Undefined option '" << flg << "'";
		if (type == CommandType::EXEC) std::cerr << " for command 'exec'" << std::endl;
		else if (type == CommandType::TOP) std::cerr << " for command 'top'" << std::endl;
		else std::cerr << std::endl;
    }
    
    
}
