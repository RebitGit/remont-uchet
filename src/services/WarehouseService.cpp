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
    std::cerr << "[WRITEOFF] writeOff OK\n";

    const char* sql =
        "INSERT INTO part_usage(order_id, part_id, quantity) VALUES(?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) {
        std::cerr << "[WRITEOFF] db is null\n";
        return false;
    }

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
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
    if (rc != SQLITE_DONE) {
        std::cerr << "[WRITEOFF] insert failed: rc=" << rc
                  << " err=" << sqlite3_errmsg(db) << "\n";
    } else {
        std::cerr << "[WRITEOFF] insert OK\n";
    }

    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) return false;

    Logger::instance().log(userId,
        "Списание запчасти: " + part.name + " x" + std::to_string(quantity),
        "part", partId);

    return true;
}

std::vector<Part> WarehouseService::getLowStockParts() {
    return PartRepository::instance().getLowStock();
}

}