#include <common/net/packet.hh>
#include <string.h>

namespace common::net {

    Packet readPacketFromBytes (const std::vector <char> & datas) {
	Packet packet;
	const char* current = reinterpret_cast <const char*> (datas.data ());
	packet.energy_pp0 = reinterpret_cast<const float*>(current)[0];
	packet.energy_pp1 = reinterpret_cast<const float*>(current)[1]; 
	packet.energy_pkg = reinterpret_cast<const float*>(current)[2];
	packet.energy_dram = reinterpret_cast<const float*>(current)[3]; 
	packet.energy_psys = reinterpret_cast<const float*>(current)[4];
	packet.core_temp   = reinterpret_cast<const unsigned int*> (current + (sizeof (float) * 5))[0];
		
	current = current + (5 * sizeof(float) + sizeof (int));
	
	int cpuFreqSize = *reinterpret_cast<const int*> (current);
	current = current + sizeof (int);
	if (cpuFreqSize != 0) {
	    packet.cpuFreq.resize (cpuFreqSize);
	    memcpy (packet.cpuFreq.data (), current, sizeof (int) * cpuFreqSize);
	    current = current + (sizeof (int) * cpuFreqSize);
	}
	
	int metricSizes = *reinterpret_cast<const int*> (current);
	const Metric * globalMetrics = reinterpret_cast <const Metric*> (current + sizeof(int));
    
	readMetricsFromBytes (packet.globalMetrics, metricSizes, globalMetrics);

	current = reinterpret_cast <const char*> (globalMetrics) + (sizeof(Metric) * metricSizes);
	short nbcgroup = *reinterpret_cast<const short*> (current);
	current += sizeof (short);

	packet.cgroupPackets.resize (nbcgroup);
	for (int i = 0 ; i < nbcgroup ; i++) {
	    current = readCgroup (packet.cgroupPackets[i], current);
	}

	return packet;
    }


    void readMetricsFromBytes (std::vector<Metric> & metrics, int nbMetrics, const Metric* data) {
	metrics.resize (nbMetrics);
	memcpy (metrics.data (), data, sizeof(Metric) * nbMetrics);
    }


    const char* readCgroup (CgroupPacket & packet, const char* datas) {
	int idSize = *reinterpret_cast<const int*> (datas);
	const char* current = datas + sizeof (int);

	packet.id.resize (idSize, ' ');

	memcpy (&packet.id[0], current, idSize);
	current += idSize;

	int metricSize = *reinterpret_cast<const int*> (current);
	current += sizeof (int);

	readMetricsFromBytes (packet.metrics, metricSize, reinterpret_cast<const Metric*> (current));

	return current + (sizeof(Metric) * metricSize);
    }

    void writePacketToBytes (const Packet & packet, std::vector <char> & result) {
	result.resize ((5 * sizeof (float)) + sizeof (unsigned int));
	reinterpret_cast<float*> (result.data ())[0] = packet.energy_pp0;
	reinterpret_cast<float*> (result.data ())[1] = packet.energy_pp1;
	reinterpret_cast<float*> (result.data ())[2] = packet.energy_pkg;
	reinterpret_cast<float*> (result.data ())[3] = packet.energy_dram;
	reinterpret_cast<float*> (result.data ())[4] = packet.energy_psys;
	reinterpret_cast<unsigned int*> (result.data () + (sizeof(float) * 5))[0] = packet.core_temp;

	auto oldSize = result.size ();
	result.resize (oldSize + sizeof(int) + (packet.cpuFreq.size () * sizeof (int)));
	reinterpret_cast <int*> (result.data () + oldSize)[0] = static_cast<int> (packet.cpuFreq.size ());
	if (packet.cpuFreq.size () != 0) {
	    memcpy (result.data () + oldSize + sizeof (int), packet.cpuFreq.data (), sizeof (int) * packet.cpuFreq.size ());
	}
	
	writeMetricsToBytes (result, packet.globalMetrics);

	oldSize = result.size ();
	result.resize (result.size () + sizeof(short));
	*reinterpret_cast<short*>(result.data () + oldSize) = packet.cgroupPackets.size ();
    
	for (int i = 0 ; i < packet.cgroupPackets.size (); i++) {
	    writeCgroupToBytes (result, packet.cgroupPackets[i]);
	}
    }
    

    std::vector<char> writePacketToBytes (const Packet & packet) {
	std::vector <char> result;
	writePacketToBytes (packet, result);

	return result;
    }

    void writeMetricsToBytes (std::vector<char> & result, const std::vector<Metric> & metrics) {
	auto oldSize = result.size ();
	result.resize (result.size () + sizeof(int) + (sizeof(Metric) * metrics.size ()));
	*reinterpret_cast<int*>(result.data () + oldSize) = metrics.size ();

	memcpy (result.data () + oldSize + sizeof (int), metrics.data (), sizeof(Metric) * metrics.size ());
    }

    void writeCgroupToBytes (std::vector <char> & result, const CgroupPacket & packet) {
	auto oldSize = result.size ();
	result.resize (result.size () + sizeof (int) + packet.id.length ());

	*reinterpret_cast<int*>(result.data () + oldSize) = packet.id.length ();
	memcpy (result.data () + oldSize + sizeof (int), packet.id.data (), packet.id.length ());


	writeMetricsToBytes (result, packet.metrics);
    }

}

std::ostream& operator<< (std::ostream& stream, const common::net::Packet & packet) {
    stream << "Packet : ";
    stream << "{energy_pp0 : " << packet.energy_pp0 << ", energy_pp1 : " << packet.energy_pp1 << ", energy_pkg : " << packet.energy_pkg << ", energy_dram : " << packet.energy_dram << ", energy_psys : " << packet.energy_psys << "}" << std::endl;
    stream << "{";
    for (int i = 0 ; i < packet.globalMetrics.size () ; i++) {
	if (i != 0) stream << ", ";
	stream << packet.globalMetrics[i].id << ": " << packet.globalMetrics[i].value;
    }
    stream << "}" << std::endl;
    for (auto & p : packet.cgroupPackets ) {
	stream << "cgroup : " << p.id << std::endl;
	stream << "{";
	for (int i = 0 ; i < p.metrics.size () ; i++) {
	    if (i != 0) stream << ", ";
	    stream << p.metrics[i].id << ": " << p.metrics [i].value;
	}
	stream << "}" << std::endl;
    }

    return stream;
}
