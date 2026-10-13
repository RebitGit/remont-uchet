#include "services/WarehouseService.h"
#include "repositories/PartRepository.h"
#include "core/DatabaseManager.h"
#include "core/Logger.h"
#include <sqlite3.h>
#include <iostream>

namespace remont {

WarehouseService& WarehouseService::instance() {
    static WarehouseService inst;
    return inst;
}

bool WarehouseService::writeOffPart(int partId, int quantity, int orderId, int userId) {
    if (quantity <= 0) {
        std::cerr << "[WRITEOFF] quantity <= 0\n";
        return false;
    }

    Part part = PartRepository::instance().findById(partId);
    if (part.id == 0) {
        std::cerr << "[WRITEOFF] part not found\n";
        return false;
    }
    if (part.quantity < quantity) {
        std::cerr << "[WRITEOFF] not enough stock: have=" << part.quantity
                  << ", want=" << quantity << "\n";
        return false;
    }

    if (!PartRepository::instance().writeOff(partId, quantity)) {
        std::cerr << "[WRITEOFF] PartRepository::writeOff failed\n";
        return false;
    }

    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) {
        std::cerr << "[WRITEOFF] db is null\n";
        return false;
    }

    const char* sqlInsert =
        "INSERT INTO part_usage(order_id, part_id, quantity) VALUES(?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sqlInsert, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[WRITEOFF] prepare failed: " << sqlite3_errmsg(db) << "\n";
        return false;
    }

    if (orderId > 0) {
        sqlite3_bind_int(stmt, 1, orderId);
    } else {
        sqlite3_bind_null(stmt, 1);
    }
    sqlite3_bind_int(stmt, 2, partId);
    sqlite3_bind_int(stmt, 3, quantity);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        std::cerr << "[WRITEOFF] insert failed\n";
        return false;
    }

    if (orderId > 0) {
        const char* sqlRecalc =
            "UPDATE orders SET "
            "parts_cost = (SELECT IFNULL(SUM(pu.quantity * p.price), 0) "
            "              FROM part_usage pu JOIN parts p ON pu.part_id = p.id "
            "              WHERE pu.order_id = ?), "
            "total_cost = work_cost + (SELECT IFNULL(SUM(pu.quantity * p.price), 0) "
            "                          FROM part_usage pu JOIN parts p ON pu.part_id = p.id "
            "                          WHERE pu.order_id = ?) "
            "WHERE id = ?;";

        sqlite3_stmt* stmtRecalc = nullptr;
        if (sqlite3_prepare_v2(db, sqlRecalc, -1, &stmtRecalc, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmtRecalc, 1, orderId);
            sqlite3_bind_int(stmtRecalc, 2, orderId);
            sqlite3_bind_int(stmtRecalc, 3, orderId);
            sqlite3_step(stmtRecalc);
            sqlite3_finalize(stmtRecalc);
        }
    }

    Logger::instance().log(userId,
        "Списание запчасти: " + part.name + " x" + std::to_string(quantity),
        "part", partId);

    return true;
}

std::vector<Part> WarehouseService::getLowStockParts() {
    return PartRepository::instance().getLowStock();
}

}