#pragma once
#include <string>
#include "core/Types.h"

namespace remont {

struct Order {
    int id = 0;
    std::string orderNumber;
    int clientId = 0;
    int deviceId = 0;
    int userId = 0;
    OrderStatus status = OrderStatus::Accepted;
    std::string receivedAt;
    std::string completedAt;
    std::string description;
    double totalCost = 0.0;

    std::string toString() const;
};

}