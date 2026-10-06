#include "services/WarehouseService.h"
#include "repositories/PartRepository.h"
#include "core/DatabaseManager.h"
#include "core/Logger.h"
#include <sqlite3.h>

namespace remont {

WarehouseService& WarehouseService::instance() {
    static WarehouseService inst;
    return inst;
}

bool WarehouseService::writeOffPart(int partId, int quantity, int orderId, int userId) {
    if (quantity <= 0) return false;

    Part part = PartRepository::instance().findById(partId);
    if (part.id == 0) return false;
    if (part.quantity < quantity) return false;

    if (!PartRepository::instance().writeOff(partId, quantity)) return false;

    const char* sql =
        "INSERT INTO part_usage(order_id, part_id, quantity) VALUES(?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return false;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, orderId);
    sqlite3_bind_int(stmt, 2, partId);
    sqlite3_bind_int(stmt, 3, quantity);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);

    if (ok) {
        Logger::instance().log(userId,
            "Списание запчасти: " + part.name + " x" + std::to_string(quantity),
            "part", partId);
    }

    return ok;
}

std::vector<Part> WarehouseService::getLowStockParts() {
    return PartRepository::instance().getLowStock();
}

}