#pragma once
#include <string>
#include <sqlite3.h>

namespace remont {

class DatabaseManager {
public:
    static DatabaseManager& instance();

    bool open(const std::string& path);
    void close();
    sqlite3* handle() const { return db_; }

    bool execute(const std::string& sql);
    bool execFile(const std::string& path);

private:
    DatabaseManager() = default;
    ~DatabaseManager();
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    sqlite3* db_ = nullptr;
};

}