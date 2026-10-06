#pragma once
#include "models/Device.h"

namespace remont {

class DeviceRepository {
public:
    static DeviceRepository& instance();

    bool add(Device& device);
    bool update(const Device& device);
    Device findById(int id);

private:
    DeviceRepository() = default;
};

}