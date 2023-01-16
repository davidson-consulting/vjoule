#include <common/cgroup/cgroup.hh>

namespace common::cgroup {

    Cgroup::Cgroup (const std::string& name) : _name (name)
    {}

    const std::string& Cgroup::getName () const {
	return this-> _name;
    }

}

std::ostream& operator<< (std::ostream& stream, const common::cgroup::Cgroup & group) {
    std::cout << "cg(" << group.getName () << ")";
    return stream;
}
