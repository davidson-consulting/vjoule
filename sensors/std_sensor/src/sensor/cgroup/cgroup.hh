#pragma once
#include <iostream>

namespace sensor::cgroup {

    class Cgroup {
    private : 

	// The name of the cgroup
	std::string _name;

    public :
	
	/**
	 * Create a cgroup from a name
	 */
	Cgroup (const std::string & name);

	/**
	 * @returns: the name of the cgroup
	 */
	const std::string& getName () const;
	
    };

}

std::ostream& operator<< (std::ostream& stream, const sensor::cgroup::Cgroup & group);
