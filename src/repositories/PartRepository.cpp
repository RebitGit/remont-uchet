#include "repositories/PartRepository.h"
#include "core/DatabaseManager.h"
#include <sqlite3.h>

namespace remont {

PartRepository& PartRepository::instance() {
    static PartRepository inst;
    return inst;
}

Part PartRepository::readRow(sqlite3_stmt* stmt) {
    Part p;
    p.id = sqlite3_column_int(stmt, 0);
    const char* n = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    p.name = n ? n : "";
    const char* a = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    p.article = a ? a : "";
    p.quantity = sqlite3_column_int(stmt, 3);
    p.price = sqlite3_column_double(stmt, 4);
    p.minQuantity = sqlite3_column_int(stmt, 5);
    return p;
}

bool PartRepository::add(Part& part) {
    const char* sql =
        "INSERT INTO parts(name, article, quantity, price, min_quantity) "
        "VALUES(?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return false;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, part.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, part.article.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, part.quantity);
    sqlite3_bind_double(stmt, 4, part.price);
    sqlite3_bind_int(stmt, 5, part.minQuantity);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    if (ok) part.id = static_cast<int>(sqlite3_last_insert_rowid(db));

    sqlite3_finalize(stmt);
    return ok;
}

bool PartRepository::update(const Part& part) {
    const char* sql =
        "UPDATE parts SET name = ?, article = ?, quantity = ?, "
        "price = ?, min_quantity = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return false;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, part.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, part.article.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, part.quantity);
    sqlite3_bind_double(stmt, 4, part.price);
    sqlite3_bind_int(stmt, 5, part.minQuantity);
    sqlite3_bind_int(stmt, 6, part.id);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool PartRepository::remove(int id) {
    const char* sql = "DELETE FROM parts WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return false;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, id);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool PartRepository::writeOff(int partId, int quantity) {
    const char* sql =
        "UPDATE parts SET quantity = quantity - ? "
        "WHERE id = ? AND quantity >= ?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return false;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, quantity);
    sqlite3_bind_int(stmt, 2, partId);
    sqlite3_bind_int(stmt, 3, quantity);

    int rc = sqlite3_step(stmt);
    bool ok = (rc == SQLITE_DONE) && (sqlite3_changes(db) > 0);

    sqlite3_finalize(stmt);
    return ok;
}

Part PartRepository::findById(int id) {
    Part part;
    const char* sql =
        "SELECT id, name, article, quantity, price, min_quantity "
        "FROM parts WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return part;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return part;
    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        part = readRow(stmt);
    }
    sqlite3_finalize(stmt);
    return part;
}

Part PartRepository::findByArticle(const std::string& article) {
    Part part;
    const char* sql =
        "SELECT id, name, article, quantity, price, min_quantity "
        "FROM parts WHERE article = ? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return part;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return part;
    sqlite3_bind_text(stmt, 1, article.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        part = readRow(stmt);
    }
    sqlite3_finalize(stmt);
    return part;
}

std::vector<Part> PartRepository::getAll() {
    std::vector<Part> result;
    const char* sql =
        "SELECT id, name, article, quantity, price, min_quantity "
        "FROM parts ORDER BY name;";
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

std::vector<Part> PartRepository::getLowStock() {
    std::vector<Part> result;
    const char* sql =
        "SELECT id, name, article, quantity, price, min_quantity "
        "FROM parts WHERE quantity <= min_quantity ORDER BY name;";
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

}