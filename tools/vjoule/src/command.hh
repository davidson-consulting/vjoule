#pragma once

#include <vector>
#include <string>


namespace tools::vjoule {

    enum class CommandType : int {
	EXEC = 0,
	PROFILE = 1,
	NONE = 2
    };

#define VERSION_FLAG "version"
#define VERSION_FLAG_SHORT "v"
#define HELP_FLAG "help"
#define HELP_FLAG_SHORT "h"

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
	 * Parse an output option at index i
	 * @returns: the content of the output option
	 */
	std::string parseOutput (uint64_t i) const;
	
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