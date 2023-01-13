#include <sensor/cgroup/cgroup.hh>

namespace sensor::cgroup {

    Cgroup::Cgroup (const std::string& name) : _name (name)
    {}

    const std::string& Cgroup::getName () const {
	return this-> _name;
    }

}

std::ostream& operator<< (std::ostream& stream, const sensor::cgroup::Cgroup & group) {
    std::cout << "cg(" << group.getName () << ")";
    return stream;
}
