#include "models/Device.h"
#include <sstream>

namespace remont {

std::string Device::toString() const {
    std::ostringstream os;
    os << "Device{id=" << id
       << ", model=" << model
       << ", sn=" << serialNumber
       << ", type=" << deviceType
       << "}";
    return os.str();
}

}