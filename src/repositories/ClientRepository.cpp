#include "repositories/ClientRepository.h"
#include "core/DatabaseManager.h"
#include <sqlite3.h>

namespace remont {

ClientRepository& ClientRepository::instance() {
    static ClientRepository inst;
    return inst;
}

bool ClientRepository::add(Client& client) {
    const char* sql =
        "INSERT INTO clients(full_name, phone, email) VALUES(?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return false;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, client.fullName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, client.phone.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, client.email.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    if (ok) client.id = static_cast<int>(sqlite3_last_insert_rowid(db));

    sqlite3_finalize(stmt);
    return ok;
}

bool ClientRepository::update(const Client& client) {
    const char* sql =
        "UPDATE clients SET full_name = ?, phone = ?, email = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return false;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, client.fullName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, client.phone.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, client.email.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, client.id);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool ClientRepository::remove(int id) {
    const char* sql = "DELETE FROM clients WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return false;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, id);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

Client ClientRepository::findById(int id) {
    Client client;
    const char* sql =
        "SELECT id, full_name, phone, email, created_at FROM clients WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return client;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return client;
    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        client.id = sqlite3_column_int(stmt, 0);
        const char* fn = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        client.fullName = fn ? fn : "";
        const char* ph = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        client.phone = ph ? ph : "";
        const char* em = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        client.email = em ? em : "";
        const char* ca = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        client.createdAt = ca ? ca : "";
    }
    sqlite3_finalize(stmt);
    return client;
}

Client ClientRepository::findByPhone(const std::string& phone) {
    Client client;
    const char* sql =
        "SELECT id, full_name, phone, email, created_at FROM clients WHERE phone = ? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return client;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return client;
    sqlite3_bind_text(stmt, 1, phone.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        client.id = sqlite3_column_int(stmt, 0);
        const char* fn = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        client.fullName = fn ? fn : "";
        const char* ph = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        client.phone = ph ? ph : "";
        const char* em = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        client.email = em ? em : "";
        const char* ca = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        client.createdAt = ca ? ca : "";
    }
    sqlite3_finalize(stmt);
    return client;
}

std::vector<Client> ClientRepository::getAll() {
    std::vector<Client> result;
    const char* sql =
        "SELECT id, full_name, phone, email, created_at FROM clients ORDER BY id DESC;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return result;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return result;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Client c;
        c.id = sqlite3_column_int(stmt, 0);
        const char* fn = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        c.fullName = fn ? fn : "";
        const char* ph = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        c.phone = ph ? ph : "";
        const char* em = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        c.email = em ? em : "";
        const char* ca = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        c.createdAt = ca ? ca : "";
        result.push_back(c);
    }
    sqlite3_finalize(stmt);
    return result;
}

}