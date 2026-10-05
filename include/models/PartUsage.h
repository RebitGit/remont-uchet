#pragma once
#include <string>

namespace remont {

struct PartUsage {
    int id = 0;
    int orderId = 0;
    int partId = 0;
    int quantity = 0;
    std::string usedAt;

    std::string toString() const;
};

}