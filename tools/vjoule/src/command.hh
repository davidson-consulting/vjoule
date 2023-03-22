#pragma once

#include <vector>
#include <string>

#define __VJOULE_VERSION__ "1.1.0"

namespace tools::vjoule {

    struct CommandError {};
    
    enum class CommandType : int {
	EXEC = 0,
	PROFILE = 1,
	TOP = 2,
	NONE = 3
    };

#define VERBOSE_FLAG "verbose"
#define VERSION_FLAG "version"
#define VERSION_FLAG_SHORT "v"
#define HELP_FLAG "help"
#define HELP_FLAG_SHORT "h"

#define PID_FLAG "pids"    
#define CPU_FLAG "no-cpu"
#define RAM_FLAG "no-ram"
#define GPU_FLAG "no-gpu"

#define NVIDIA_FLAG "no-nvidia"
#define NVIDIA_FLAG_SHORT "nv"
    
#define RAPL_FLAG "no-rapl"
#define RAPL_FLAG_SHORT "nr"

#define OUTPUT_FLAG "output"
#define OUTPUT_FLAG_SHORT "o"
    
    struct CommandLine {
	CommandType type;
	bool verbose;
	bool cpu;
	bool ram;
	bool gpu;
	bool rapl;
	bool nvidia;
	std::string output;
	std::vector <std::string> subCmd;
	std::vector <uint64_t> pids;
    };


/**
 * The command parser of the client
 */
    class CommandParser {
    private : 

	// The command line that was parsed
	CommandLine _content;

	// The number of arguments
	int _argc;

	// The list of arguments
	char ** _argv;
    

    public: 

	/**
	 * @params: 
	 *    - argc: the number of arguments
	 *    - argv: the list of arguments
	 */
	CommandParser (int argc, char ** argv);
    
	/**
	 * @return: the command line parsed by the command line parser
	 */
	const CommandLine &getCommandLine () const;
	
    private:

	/**
	 * Parse the command line
	 */
	void parse ();

	/**
	 * Parse the profile sub command
	 */
	void parseProfile ();
	
	/**
	 * Parse the exec subcommand
	 * @params: 
	 *    - withKeyword: exec was written down
	 */
	void parseExec (bool withKeyword);

	/**
	 * Parse the top subcommand
	 */
	void parseTop ();

	/**
	 * Parse an output option at index i
	 * @returns: 
	 *    - the number of arguments that were read
	 *    - res: the content of the output option
	 */
	uint64_t parseOutput (uint64_t i, std::string & res) const;

	/**
	 * Parse a list of pid at index i
	 * @returns: 
	 *    - the number of arguments that were read
	 *    - res: the list of pids that were read
	 */
	uint64_t parsePidList (uint64_t i, std::vector <uint64_t> & pids) const;
	
	/**
	 * Print help command
	 */
	void printHelp (CommandType type, bool full = false) const;

	/**
	 * Print version command
	 */
	void printVersion () const;

	/**
	 * Print an error
	 */
	void printError (CommandType type, const std::string & flg) const;

	/**
	 * @returns: the flag if it is a flag 
	 * @info: for known flag return the long version
	 */
	std::string isFlag (const std::string & arg) const;

	/**
	 * Check that parsed command line is consistent
	 */
	void checkConsistency () const;

    };

    
}
