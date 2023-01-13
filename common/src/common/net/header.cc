#include <common/net/header.hh>
#include <common/utils/print.hh>
#include <string.h>

namespace common::net {

    Header readHeaderFromBytes (const std::vector <char> & datas) {
	Header head;
	head.nbCores = *reinterpret_cast<const int*> (datas.data ());
	const char* current = datas.data () + sizeof(int);

	head.simulation = *reinterpret_cast<const bool*> (current);
	current += sizeof (bool);

	head.limits = *reinterpret_cast<const PowerLimits*> (current);
	current += sizeof (PowerLimits);
	
	int nbPowerMetrics = *reinterpret_cast<const int*>(current);
	current = current + sizeof (int);
	for (int i = 0 ; i < nbPowerMetrics ; i++) {
	    short power = *reinterpret_cast<const short*>(current);
	    current = current + sizeof (short);
	    head.powerByCpus.push_back (power);
	}
	
	int nbNames = *reinterpret_cast<const int*> (current);
	current = current + sizeof (int);
	for (int i = 0 ; i < nbNames ; i++) {
	    auto id = *reinterpret_cast<const short*> (current);
	    current += sizeof (short);
	    auto nameSize = *reinterpret_cast <const int*> (current);
	    current += sizeof(int);

	    std::string name;
	    name.resize (nameSize, ' ');
	    memcpy (&name[0], current, nameSize);
	    current += nameSize;

	    head.metricNames.emplace (id, name);
	}

	return head;
    }

    std::vector<char> writeHeaderToBytes (const Header & head) {
	std::vector <char> result;
	result.resize (result.size () + sizeof (int) + sizeof (PowerLimits) + sizeof (bool));
	
	*reinterpret_cast<int*> (result.data ()) = head.nbCores;
	*reinterpret_cast<bool*> (result.data () + sizeof (int)) = head.simulation;
	*reinterpret_cast<PowerLimits*> (result.data () + sizeof(int) + sizeof (bool)) = head.limits;
	
	auto oldSize_ = result.size ();
	result.resize (oldSize_ + (sizeof (short) * head.powerByCpus.size ()) + sizeof (int));
	*reinterpret_cast<int*> (result.data () + oldSize_ + sizeof (int)) = head.powerByCpus.size ();
	for (int i = 0 ; i < head.powerByCpus.size () ; i++) {
	    *reinterpret_cast<short*> (result.data () + oldSize_ + sizeof (int) + (sizeof(short) * i)) = head.powerByCpus[i];
	}
	
	oldSize_ = result.size ();	
	result.resize (result.size () + sizeof (int));
	*reinterpret_cast <int*> (result.data () + (oldSize_)) = head.metricNames.size ();
	
	for (auto & name : head.metricNames) {
	    auto oldSize = result.size ();
	    result.resize (oldSize + sizeof(short) + sizeof (int) + name.second.length ());
	    *reinterpret_cast <short*> (result.data () + oldSize) = name.first;
	    *reinterpret_cast<int*> (result.data () + oldSize + sizeof (short)) = (int) name.second.length ();
	    memcpy (result.data () + oldSize + sizeof (short) + sizeof (int), name.second.data (), name.second.length ());
	}

	return result;
    }

    InversedHeader reverse (const Header & head) {
	InversedHeader result;
	for (auto& it : head.metricNames) {
	    result.metricIds.emplace (it.second, it.first);
	}

	return result;
    }

    Header reverse (const InversedHeader & head) {
	Header result;
	for (auto& it : head.metricIds) {
	    result.metricNames.emplace (it.second, it.first);
	}

	return result;
    }

}

std::ostream & operator<< (std::ostream & stream, const common::net::Header & head) {
    stream << "{";
    stream << "nb_cores : " << head.nbCores << ", power : [";
    for (int i = 0 ; i < head.powerByCpus.size () ; i++) {
	if (i != 0) stream << ", ";
	stream << head.powerByCpus [i];
    }
    stream << "], metrics : {";
    
    int i = 0;
    for (auto & it : head.metricNames) {
	if (i != 0) stream << ", ";
	i += 1;
	stream << it.first << ": " << it.second ;
    }
    stream << "} }";

    return stream;
}

std::ostream & operator<< (std::ostream & stream, const common::net::InversedHeader & head) {
    stream << "{";    
    int i = 0;
    for (auto & it : head.metricIds) {
	if (i != 0) stream << ", ";
	i += 1;
	stream << it.first << ": " << it.second ;
    }
    stream << "}";

    return stream;
}

template <typename Map>
bool map_compare (Map const &lhs, Map const &rhs) {
    // No predicate needed because there is operator== for pairs already.
    return lhs.size() == rhs.size()
        && std::equal(lhs.begin(), lhs.end(),
                      rhs.begin());
}

bool operator!= (const common::net::Header & left, const common::net::Header & right) {
    return map_compare (left.metricNames, right.metricNames) != 1;
}
