#include "repositories/OrderRepository.h"
#include "core/DatabaseManager.h"
#include <sqlite3.h>
#include <sstream>
#include <iomanip>

namespace remont {

OrderRepository& OrderRepository::instance() {
    static OrderRepository inst;
    return inst;
}

std::string OrderRepository::generateNumber() {
    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return "#1";

    const char* sql = "SELECT COUNT(*) FROM orders;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return "#1";
    }

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    int next = count + 1;

    std::ostringstream os;
    os << "#" << next;
    return os.str();
}

Order OrderRepository::readRow(sqlite3_stmt* stmt) {
    Order o;
    o.id = sqlite3_column_int(stmt, 0);
    const char* num = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    o.orderNumber = num ? num : "";
    o.clientId = sqlite3_column_int(stmt, 2);
    o.deviceId = sqlite3_column_int(stmt, 3);
    o.userId = sqlite3_column_int(stmt, 4);
    o.masterId = sqlite3_column_int(stmt, 5);

    const char* s = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
    std::string statusStr = s ? s : "";
    if (statusStr == "Принят") o.status = OrderStatus::Accepted;
    else if (statusStr == "Диагностика") o.status = OrderStatus::Diagnostics;
    else if (statusStr == "В ремонте") o.status = OrderStatus::InRepair;
    else if (statusStr == "Ожидает запчасть") o.status = OrderStatus::WaitingPart;
    else if (statusStr == "Готов") o.status = OrderStatus::Ready;
    else if (statusStr == "Выдан") o.status = OrderStatus::Issued;

    const char* ra = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
    o.receivedAt = ra ? ra : "";
    const char* ca = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
    o.completedAt = ca ? ca : "";
    const char* d = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
    o.description = d ? d : "";
    o.totalCost = sqlite3_column_double(stmt, 10);
    return o;
}

bool OrderRepository::add(Order& order) {
    if (order.orderNumber.empty()) order.orderNumber = generateNumber();

    const char* sql =
        "INSERT INTO orders(order_number, client_id, device_id, user_id, master_id, "
        "status, description, total_cost) VALUES(?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return false;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, order.orderNumber.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, order.clientId);
    sqlite3_bind_int(stmt, 3, order.deviceId);
    sqlite3_bind_int(stmt, 4, order.userId);
    if (order.masterId > 0) {
        sqlite3_bind_int(stmt, 5, order.masterId);
    } else {
        sqlite3_bind_null(stmt, 5);
    }
    std::string statusStr = statusToString(order.status);
    sqlite3_bind_text(stmt, 6, statusStr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, order.description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 8, order.totalCost);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    if (ok) order.id = static_cast<int>(sqlite3_last_insert_rowid(db));

    sqlite3_finalize(stmt);
    return ok;
}

bool OrderRepository::update(const Order& order) {
    const char* sql =
        "UPDATE orders SET status = ?, description = ?, total_cost = ?, "
        "completed_at = ?, master_id = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return false;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    std::string statusStr = statusToString(order.status);
    sqlite3_bind_text(stmt, 1, statusStr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, order.description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, order.totalCost);
    sqlite3_bind_text(stmt, 4, order.completedAt.c_str(), -1, SQLITE_TRANSIENT);
    if (order.masterId > 0) {
        sqlite3_bind_int(stmt, 5, order.masterId);
    } else {
        sqlite3_bind_null(stmt, 5);
    }
    sqlite3_bind_int(stmt, 6, order.id);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool OrderRepository::updateStatus(int orderId, OrderStatus status) {
    const char* sql = "UPDATE orders SET status = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return false;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    std::string statusStr = statusToString(status);
    sqlite3_bind_text(stmt, 1, statusStr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, orderId);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool OrderRepository::remove(int id) {
    const char* sql = "DELETE FROM orders WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return false;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, id);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

Order OrderRepository::findById(int id) {
    Order order;
    const char* sql =
        "SELECT id, order_number, client_id, device_id, user_id, master_id, "
        "status, received_at, completed_at, description, total_cost "
        "FROM orders WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return order;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return order;
    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        order = readRow(stmt);
    }
    sqlite3_finalize(stmt);
    return order;
}

std::vector<Order> OrderRepository::getAll() {
    std::vector<Order> result;
    const char* sql =
        "SELECT id, order_number, client_id, device_id, user_id, master_id, "
        "status, received_at, completed_at, description, total_cost "
        "FROM orders ORDER BY id DESC;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return result;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return result;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        result.push_back(readRow(stmt));
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<Order> OrderRepository::getByStatus(OrderStatus status) {
    std::vector<Order> result;
    std::string statusStr = statusToString(status);
    const char* sql =
        "SELECT id, order_number, client_id, device_id, user_id, master_id, "
        "status, received_at, completed_at, description, total_cost "
        "FROM orders WHERE status = ? ORDER BY id DESC;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return result;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return result;
    sqlite3_bind_text(stmt, 1, statusStr.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        result.push_back(readRow(stmt));
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<Order> OrderRepository::getByClient(int clientId) {
    std::vector<Order> result;
    const char* sql =
        "SELECT id, order_number, client_id, device_id, user_id, master_id, "
        "status, received_at, completed_at, description, total_cost "
        "FROM orders WHERE client_id = ? ORDER BY id DESC;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return result;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return result;
    sqlite3_bind_int(stmt, 1, clientId);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        result.push_back(readRow(stmt));
    }
    sqlite3_finalize(stmt);
    return result;
}

}