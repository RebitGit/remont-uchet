#include "core/DatabaseManager.h"
#include <iostream>
#include <fstream>
#include <sstream>

namespace remont {

DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager inst;
    return inst;
}

DatabaseManager::~DatabaseManager() { close(); }

bool DatabaseManager::open(const std::string& path) {
    if (db_) close();
    int rc = sqlite3_open(path.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::cerr << "Не удалось открыть БД: " << sqlite3_errmsg(db_) << "\n";
        db_ = nullptr;
        return false;
    }
    execute("PRAGMA foreign_keys = ON;");
    execute("PRAGMA encoding = 'UTF-8';");
    return true;
}

void DatabaseManager::close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool DatabaseManager::execute(const std::string& sql) {
    if (!db_) return false;
    char* err = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL ошибка: " << (err ? err : "unknown") << "\n";
        sqlite3_free(err);
        return false;
    }
    return true;
}

bool DatabaseManager::execFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "Не удалось открыть SQL-файл: " << path << "\n";
        return false;
    }
    std::stringstream ss;
    ss << f.rdbuf();
    return execute(ss.str());
}

}