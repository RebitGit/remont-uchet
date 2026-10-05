#pragma once
#include <string>

namespace remont {

struct Device {
    int id = 0;
    std::string model;
    std::string serialNumber;
    std::string deviceType;

    std::string toString() const;
};

}