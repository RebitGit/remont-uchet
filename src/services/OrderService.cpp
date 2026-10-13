#include "services/OrderService.h"
#include "repositories/ClientRepository.h"
#include "repositories/DeviceRepository.h"
#include "repositories/OrderRepository.h"
#include "pdf/PDFGenerator.h"
#include "core/ConfigManager.h"
#include "core/DatabaseManager.h"
#include "core/Logger.h"

#include <sqlite3.h>

namespace remont {

OrderService& OrderService::instance() {
    static OrderService inst;
    return inst;
}

bool OrderService::createOrder(const std::string& clientFullName,
                               const std::string& clientPhone,
                               const std::string& clientEmail,
                               const std::string& deviceType,
                               const std::string& deviceModel,
                               const std::string& deviceSerial,
                               const std::string& description,
                               double totalCost,
                               int userId,
                               Order& outOrder,
                               int masterId) {
    if (clientFullName.empty() || clientPhone.empty()) return false;
    if (deviceModel.empty()) return false;

    Client client = ClientRepository::instance().findByPhone(clientPhone);
    if (client.id == 0) {
        client.fullName = clientFullName;
        client.phone = clientPhone;
        client.email = clientEmail;
        if (!ClientRepository::instance().add(client)) return false;
    }

    Device device;
    device.model = deviceModel;
    device.serialNumber = deviceSerial;
    device.deviceType = deviceType;
    if (!DeviceRepository::instance().add(device)) return false;

    Order order;
    order.clientId = client.id;
    order.deviceId = device.id;
    order.userId = userId;
    order.masterId = masterId;
    order.status = OrderStatus::Accepted;
    order.description = description;
    order.workCost = totalCost;
    order.partsCost = 0.0;
    order.totalCost = totalCost;

    if (!OrderRepository::instance().add(order)) return false;

    sqlite3* db = DatabaseManager::instance().handle();
    if (db) {
        const char* sql =
            "UPDATE orders SET "
            "work_cost = ?, "
            "parts_cost = 0, "
            "total_cost = ? "
            "WHERE id = ?;";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_double(stmt, 1, totalCost);
            sqlite3_bind_double(stmt, 2, totalCost);
            sqlite3_bind_int(stmt, 3, order.id);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }

    Logger::instance().log(userId, "Заказ создан. Статус: Принят", "order", order.id);

    outOrder = order;
    return true;
}

bool OrderService::changeStatus(int orderId, OrderStatus newStatus) {
    Order order = OrderRepository::instance().findById(orderId);
    if (order.id == 0) return false;

    std::string oldStatusStr = statusToString(order.status);
    std::string newStatusStr = statusToString(newStatus);

    if (oldStatusStr == newStatusStr) return true;

    bool ok = OrderRepository::instance().updateStatus(orderId, newStatus);
    if (!ok) return false;

    sqlite3* db = DatabaseManager::instance().handle();
    if (db && newStatus == OrderStatus::Issued) {
        const char* sql =
            "UPDATE orders SET completed_at = CURRENT_TIMESTAMP WHERE id = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, orderId);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }

    Logger::instance().log(0, "Статус: " + newStatusStr, "order", orderId);
    return true;
}

bool OrderService::printAcceptanceAct(int orderId, const std::string& outputPath) {
    Order order = OrderRepository::instance().findById(orderId);
    if (order.id == 0) return false;

    Client client = ClientRepository::instance().findById(order.clientId);
    Device device = DeviceRepository::instance().findById(order.deviceId);

    bool ok = PDFGenerator::instance().generateAcceptanceAct(
        order, client, device, outputPath,
        ConfigManager::instance().fontPath());

    if (ok) {
        Logger::instance().log(0, "Печать акта для заказа " + order.orderNumber,
                               "order", orderId);
    }
    return ok;
}

int OrderService::cleanupOldIssuedOrders(int days) {
    if (days <= 0) return 0;

    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return 0;

    const char* sql =
        "DELETE FROM orders "
        "WHERE status = 'Выдан' "
        "AND completed_at IS NOT NULL "
        "AND completed_at <= datetime('now', '-' || ? || ' days');";

    sqlite3_stmt* stmt = nullptr;
    int deleted = 0;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, days);
        if (sqlite3_step(stmt) == SQLITE_DONE) {
            deleted = sqlite3_changes(db);
        }
        sqlite3_finalize(stmt);
    }

    if (deleted > 0) {
        Logger::instance().log(0,
            "Автоудаление: удалено заказов со статусом «Выдан» старше " +
            std::to_string(days) + " дней: " + std::to_string(deleted),
            "order", 0);
    }

    return deleted;
}

}