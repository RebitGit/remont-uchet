#pragma once
#include <vector>
#include <sqlite3.h>
#include "models/Order.h"

namespace remont {

class OrderRepository {
public:
    static OrderRepository& instance();

    bool add(Order& order);
    bool update(const Order& order);
    bool updateStatus(int orderId, OrderStatus status);
    bool remove(int id);

    Order findById(int id);
    std::vector<Order> getAll();
    std::vector<Order> getByStatus(OrderStatus status);
    std::vector<Order> getByClient(int clientId);

    std::string generateNumber();

private:
    OrderRepository() = default;

    Order readRow(sqlite3_stmt* stmt);
};

}