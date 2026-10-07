#include "core/DatabaseManager.h"
#include "core/ConfigManager.h"
#include "services/WarehouseService.h"
#include "repositories/PartRepository.h"
#include "models/Part.h"

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

    remont::Part p;
    p.name = "Тестовая запчасть";
    p.article = "TEST-001";
    p.quantity = 10;
    p.price = 100;
    p.minQuantity = 2;

    remont::PartRepository::instance().remove(
        remont::PartRepository::instance().findByArticle("TEST-001").id);

    if (!remont::PartRepository::instance().add(p)) {
        std::cout << "FAIL: не удалось создать тестовую запчасть\n";
        return 1;
    }

    std::cout << "OK: тестовая запчасть создана, id=" << p.id << "\n";
    passed++;

    bool ok = remont::WarehouseService::instance().writeOffPart(p.id, 3, 0, 0);
    if (ok) {
        std::cout << "OK: списание прошло\n";
        passed++;
    } else {
        std::cout << "FAIL: списание не прошло\n";
        failed++;
    }

    remont::Part updated = remont::PartRepository::instance().findById(p.id);
    if (updated.quantity == 7) {
        std::cout << "OK: остаток уменьшился до 7\n";
        passed++;
    } else {
        std::cout << "FAIL: остаток " << updated.quantity << " (ожидалось 7)\n";
        failed++;
    }

    bool tooMuch = remont::WarehouseService::instance().writeOffPart(p.id, 100, 0, 0);
    if (!tooMuch) {
        std::cout << "OK: попытка списать больше остатка отклонена\n";
        passed++;
    } else {
        std::cout << "FAIL: списание больше остатка прошло\n";
        failed++;
    }

    remont::PartRepository::instance().remove(p.id);

    std::cout << "\nПройдено: " << passed << ", провалено: " << failed << "\n";
    return failed == 0 ? 0 : 1;
}