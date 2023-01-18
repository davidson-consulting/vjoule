#include "reader.hh"

namespace nvidia {

    bool NvmlReader::configure () {
	return true;
    }

    void NvmlReader::poll () {
	LOG_INFO ("POLLING");
    }

    uint32_t NvmlReader::getNbDevices () const {
	return 1;
    }

    float NvmlReader::getGpuEnergy (uint32_t device) const {
	return 0;
    }

    void NvmlReader::dispose () {
	LOG_INFO ("dipose");
    }


    
}
