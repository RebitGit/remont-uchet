#include "core/DatabaseManager.h"
#include "core/ConfigManager.h"
#include "services/OrderService.h"
#include "repositories/OrderRepository.h"
#include "models/Order.h"

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

    int before = static_cast<int>(remont::OrderRepository::instance().getAll().size());

    remont::Order order;
    bool ok = remont::OrderService::instance().createOrder(
        "Тестовый Клиент", "+7-900-000-00-00", "test@test.ru",
        "Ноутбук", "TestModel", "SN-TEST",
        "Тестовое описание", 1, order);

    if (ok && order.id > 0) {
        std::cout << "OK: заказ создан, id=" << order.id
                  << ", номер=" << order.orderNumber << "\n";
        passed++;
    } else {
        std::cout << "FAIL: заказ не создан\n";
        failed++;
        std::cout << "\nПройдено: " << passed << ", провалено: " << failed << "\n";
        return 1;
    }

    int after = static_cast<int>(remont::OrderRepository::instance().getAll().size());
    if (after == before + 1) {
        std::cout << "OK: количество заказов увеличилось\n";
        passed++;
    } else {
        std::cout << "FAIL: количество заказов не изменилось ("
                  << before << " -> " << after << ")\n";
        failed++;
    }

    remont::Order found = remont::OrderRepository::instance().findById(order.id);
    if (found.id == order.id && found.orderNumber == order.orderNumber) {
        std::cout << "OK: заказ найден в БД\n";
        passed++;
    } else {
        std::cout << "FAIL: заказ не найден в БД\n";
        failed++;
    }

    std::cout << "\nПройдено: " << passed << ", провалено: " << failed << "\n";
    return failed == 0 ? 0 : 1;
}