#include "core/Logger.h"
#include "core/DatabaseManager.h"
#include <sqlite3.h>
#include <iostream>

namespace remont {

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

void Logger::log(int userId, const std::string& action,
                 const std::string& entityType, int entityId) {
    const char* sql =
        "INSERT INTO operation_log(user_id, action, entity_type, entity_id) "
        "VALUES(?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(DatabaseManager::instance().handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Logger: prepare failed\n";
        return;
    }
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_text(stmt, 2, action.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, entityType.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, entityId);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

}