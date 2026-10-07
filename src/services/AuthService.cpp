#include "services/AuthService.h"
#include "core/DatabaseManager.h"
#include "core/Logger.h"
#include <sqlite3.h>
#include <sstream>
#include <iomanip>
#include <functional>
#include <iostream>

namespace remont {

AuthService& AuthService::instance() {
    static AuthService inst;
    return inst;
}

std::string AuthService::hashPassword(const std::string& password) {
    std::hash<std::string> hasher;
    size_t h = hasher(password);
    std::ostringstream os;
    os << std::hex << std::setw(16) << std::setfill('0') << h;
    return os.str();
}

bool AuthService::authenticate(const std::string& login,
                               const std::string& password,
                               User& outUser) {
    std::cout << "[AUTH] Вход: login='" << login
              << "' password='" << password
              << "' hash='" << hashPassword(password) << "'" << std::endl;

    const char* sql =
        "SELECT id, login, password_hash, role, is_active "
        "FROM users WHERE login = ? LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) {
        std::cout << "[AUTH] БД не открыта" << std::endl;
        return false;
    }

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cout << "[AUTH] Ошибка prepare: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, login.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* loginTxt  = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* hashTxt   = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char* roleTxt   = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        int         isActive  = sqlite3_column_int(stmt, 4);

        std::string dbLogin = loginTxt ? loginTxt : "";
        std::string dbHash  = hashTxt  ? hashTxt  : "";
        std::string dbRole  = roleTxt  ? roleTxt  : "";

        std::cout << "[AUTH] Найден в БД: login='" << dbLogin
                  << "' hash='" << dbHash
                  << "' active=" << isActive << std::endl;

        std::string inputHash = hashPassword(password);
        std::cout << "[AUTH] inputHash='" << inputHash
                  << "' dbHash='" << dbHash
                  << "' равны=" << (inputHash == dbHash ? "да" : "нет")
                  << std::endl;

        if (isActive && dbHash == inputHash) {
            outUser.id           = sqlite3_column_int(stmt, 0);
            outUser.login        = dbLogin;
            outUser.passwordHash = dbHash;
            outUser.isActive     = true;

            if (dbRole == "admin")           outUser.role = Role::Admin;
            else if (dbRole == "master")     outUser.role = Role::Master;
            else if (dbRole == "warehouse")  outUser.role = Role::Warehouse;
            else                              outUser.role = Role::Operator;

            ok = true;
            std::cout << "[AUTH] УСПЕХ" << std::endl;

            Logger::instance().log(outUser.id, "Вход в систему", "user", outUser.id);
        } else {
            std::cout << "[AUTH] ОТКАЗ: "
                      << (isActive ? "хеш не совпал" : "пользователь отключён")
                      << std::endl;
        }
    } else {
        std::cout << "[AUTH] Пользователь '" << login << "' не найден в БД" << std::endl;
    }

    sqlite3_finalize(stmt);
    return ok;
}

}