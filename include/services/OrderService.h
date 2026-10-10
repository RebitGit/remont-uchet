#pragma once
#include <string>
#include "models/Order.h"

namespace remont {

class OrderService {
public:
    static OrderService& instance();

    bool createOrder(const std::string& clientFullName,
                     const std::string& clientPhone,
                     const std::string& clientEmail,
                     const std::string& deviceType,
                     const std::string& deviceModel,
                     const std::string& deviceSerial,
                     const std::string& description,
                     double totalCost,
                     int userId,
                     Order& outOrder,
                     int masterId = 0);

    bool changeStatus(int orderId, OrderStatus newStatus);

    bool printAcceptanceAct(int orderId, const std::string& outputPath);

private:
    OrderService() = default;
};

}