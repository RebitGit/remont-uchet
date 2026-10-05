#include "models/Order.h"
#include <sstream>

namespace remont {

std::string Order::toString() const {
    std::ostringstream os;
    os << "Order{id=" << id
       << ", number=" << orderNumber
       << ", client=" << clientId
       << ", device=" << deviceId
       << ", status=" << statusToString(status)
       << ", cost=" << totalCost
       << "}";
    return os.str();
}

}