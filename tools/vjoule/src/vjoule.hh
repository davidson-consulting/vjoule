#pragma once

#include <vector>
#include <common/concurrency/_.hh>
#include <common/utils/_.hh>
#include <common/net/_.hh>

namespace tools::vjoule {
	class VJoule {
		public:
			VJoule(int argc, char * argv[]);
			void run();

		private:
			int _argc;
			char ** _argv;
			std::vector <std::string> subargs;
			common::concurrency::SubProcess _child;
	};
}
