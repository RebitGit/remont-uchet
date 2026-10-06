#include "repositories/DeviceRepository.h"
#include "core/DatabaseManager.h"
#include <sqlite3.h>

namespace remont {

DeviceRepository& DeviceRepository::instance() {
    static DeviceRepository inst;
    return inst;
}

bool DeviceRepository::add(Device& device) {
    const char* sql =
        "INSERT INTO devices(model, serial_number, device_type) VALUES(?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return false;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, device.model.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, device.serialNumber.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, device.deviceType.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    if (ok) device.id = static_cast<int>(sqlite3_last_insert_rowid(db));

    sqlite3_finalize(stmt);
    return ok;
}

bool DeviceRepository::update(const Device& device) {
    const char* sql =
        "UPDATE devices SET model = ?, serial_number = ?, device_type = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return false;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, device.model.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, device.serialNumber.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, device.deviceType.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, device.id);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

Device DeviceRepository::findById(int id) {
    Device device;
    const char* sql =
        "SELECT id, model, serial_number, device_type FROM devices WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return device;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return device;
    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        device.id = sqlite3_column_int(stmt, 0);
        const char* m = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        device.model = m ? m : "";
        const char* sn = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        device.serialNumber = sn ? sn : "";
        const char* dt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        device.deviceType = dt ? dt : "";
    }
    sqlite3_finalize(stmt);
    return device;
}

}