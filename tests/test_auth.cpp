#include "core/DatabaseManager.h"
#include "core/ConfigManager.h"
#include "services/AuthService.h"
#include "models/User.h"

#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

int main() {
    auto& cfg = remont::ConfigManager::instance();
    auto& db = remont::DatabaseManager::instance();

    if (!db.open(cfg.dbPath())) {
        std::cerr << "FAIL: cannot open DB\n";
        return 1;
    }
    db.execFile(cfg.schemaPath());

    int passed = 0;
    int failed = 0;

    remont::User u;
    if (remont::AuthService::instance().authenticate("admin", "admin", u)) {
        std::cout << "OK: admin/admin проходит\n";
        passed++;
    } else {
        std::cout << "FAIL: admin/admin не проходит\n";
        failed++;
    }

    remont::User u2;
    if (!remont::AuthService::instance().authenticate("admin", "wrong", u2)) {
        std::cout << "OK: admin/wrong отклонён\n";
        passed++;
    } else {
        std::cout << "FAIL: admin/wrong прошёл\n";
        failed++;
    }

    remont::User u3;
    if (!remont::AuthService::instance().authenticate("unknown", "any", u3)) {
        std::cout << "OK: unknown отклонён\n";
        passed++;
    } else {
        std::cout << "FAIL: unknown прошёл\n";
        failed++;
    }

    std::cout << "\nПройдено: " << passed << ", провалено: " << failed << "\n";
    return failed == 0 ? 0 : 1;
}