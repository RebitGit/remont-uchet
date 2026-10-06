#include "core/DatabaseManager.h"
#include "core/ConfigManager.h"
#include "services/AuthService.h"
#include "ui/LoginDialog.h"
#include "ui/MainWindow.h"
#include "resources/utf8.h"

#include <wx/wx.h>
#include <sqlite3.h>
#include <hpdf.h>

#include <filesystem>
#include <windows.h>

using namespace remont;

namespace fs = std::filesystem;

static void seedDemoData(remont::DatabaseManager& db) {
    sqlite3_stmt* stmt = nullptr;

    sqlite3_prepare_v2(db.handle(), "SELECT COUNT(*) FROM clients;", -1, &stmt, nullptr);
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    if (count > 0) return;

    const char* clients[] = {
        "INSERT INTO clients(full_name, phone, email) VALUES('Иванов Иван Иванович', '+7 (916) 123-45-67', 'ivanov@example.com');",
        "INSERT INTO clients(full_name, phone, email) VALUES('Петрова Ольга Сергеевна', '+7 (903) 234-56-78', 'petrova@example.com');",
        "INSERT INTO clients(full_name, phone, email) VALUES('Сидоров Алексей Михайлович', '+7 (926) 345-67-89', 'sidorov@example.com');",
        "INSERT INTO clients(full_name, phone, email) VALUES('Кузнецова Мария Андреевна', '+7 (912) 456-78-90', 'kuznetsova@example.com');",
        "INSERT INTO clients(full_name, phone, email) VALUES('Новиков Дмитрий Олегович', '+7 (925) 567-89-01', 'novikov@example.com');",
        "INSERT INTO clients(full_name, phone, email) VALUES('Морозова Екатерина Ивановна', '+7 (917) 678-90-12', 'morozova@example.com');"
    };
    for (auto sql : clients) db.execute(sql);

    const char* devices[] = {
        "INSERT INTO devices(model, serial_number, device_type) VALUES('Ноутбук HP Pavilion 15', '5CD1234567', 'notebook');",
        "INSERT INTO devices(model, serial_number, device_type) VALUES('Смартфон Samsung Galaxy A52', 'SN-A52-001', 'smartphone');",
        "INSERT INTO devices(model, serial_number, device_type) VALUES('Планшет Apple iPad 10', 'SN-IPAD-10', 'tablet');",
        "INSERT INTO devices(model, serial_number, device_type) VALUES('Монитор LG 27UL500', 'SN-LG-27', 'monitor');",
        "INSERT INTO devices(model, serial_number, device_type) VALUES('Принтер Canon PIXMA', 'SN-CANON-01', 'printer');",
        "INSERT INTO devices(model, serial_number, device_type) VALUES('Ноутбук Lenovo IdeaPad 3', 'SN-LENOVO-3', 'notebook');"
    };
    for (auto sql : devices) db.execute(sql);

    const char* orders[] = {
        "INSERT INTO orders(order_number, client_id, device_id, user_id, status, description, total_cost) "
        "VALUES('ORD-2026-0001', 1, 1, 1, 'В ремонте', 'Не включается, не заряжается аккумулятор.', 3500);",

        "INSERT INTO orders(order_number, client_id, device_id, user_id, status, description, total_cost) "
        "VALUES('ORD-2026-0002', 2, 2, 1, 'Принят', 'Разбит экран, не реагирует на касания.', 0);",

        "INSERT INTO orders(order_number, client_id, device_id, user_id, status, description, total_cost) "
        "VALUES('ORD-2026-0003', 3, 3, 1, 'Готов', 'Не держит заряд, быстро разряжается.', 2800);",

        "INSERT INTO orders(order_number, client_id, device_id, user_id, status, description, total_cost) "
        "VALUES('ORD-2026-0004', 4, 4, 1, 'Диагностика', 'Нет изображения, подсветка работает.', 0);",

        "INSERT INTO orders(order_number, client_id, device_id, user_id, status, description, total_cost) "
        "VALUES('ORD-2026-0005', 5, 5, 1, 'Ожидает запчасть', 'Замятие бумаги, требуется замена ролика.', 1500);",

        "INSERT INTO orders(order_number, client_id, device_id, user_id, status, description, total_cost) "
        "VALUES('ORD-2026-0006', 6, 6, 1, 'Выдан', 'Замена клавиатуры, отремонтирован.', 4200);"
    };
    for (auto sql : orders) db.execute(sql);
}

class RemontApp : public wxApp {
public:
    bool OnInit() override {
        SetConsoleOutputCP(CP_UTF8);
        wxInitAllImageHandlers();

        auto& cfg = remont::ConfigManager::instance();
        fs::create_directories(fs::path(cfg.dbPath()).parent_path());

        auto& db = remont::DatabaseManager::instance();
        if (!db.open(cfg.dbPath())) {
            wxMessageBox(utf8::U("Не удалось открыть БД"),
                         utf8::U("Ошибка"), wxOK | wxICON_ERROR);
            return false;
        }
        db.execFile(cfg.schemaPath());

        {
            const char* checkSql =
                "SELECT password_hash FROM users WHERE login = 'admin' LIMIT 1;";
            sqlite3_stmt* stmt = nullptr;
            sqlite3_prepare_v2(db.handle(), checkSql, -1, &stmt, nullptr);
            std::string existingHash;
            bool hasAdmin = false;
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                hasAdmin = true;
                const char* h = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                existingHash = h ? h : "";
            }
            sqlite3_finalize(stmt);

            std::string adminHash = remont::AuthService::hashPassword("admin");

            if (!hasAdmin) {
                db.execute("INSERT INTO users(login, password_hash, role, is_active) "
                           "VALUES('admin', '" + adminHash + "', 'admin', 1);");
            } else if (existingHash != adminHash) {
                db.execute("UPDATE users SET password_hash = '" + adminHash +
                           "' WHERE login = 'admin';");
            }
        }

        seedDemoData(db);

        CallAfter([this]() { showLogin(); });
        return true;
    }

    void showLogin() {
        remont::LoginDialog dlg(nullptr);
        if (dlg.ShowModal() != wxID_OK) {
            ExitMainLoop();
            return;
        }

        remont::User user = dlg.getUser();

        auto* mainWindow = new remont::MainWindow(user);
        mainWindow->Bind(wxEVT_DESTROY, [this, mainWindow](wxWindowDestroyEvent&) {
            bool logout = mainWindow->logoutRequested();
            mainWindow->Destroy();
            if (logout) {
                CallAfter([this]() { showLogin(); });
            } else {
                ExitMainLoop();
            }
        });
        mainWindow->Show();
    }
};

wxIMPLEMENT_APP(RemontApp);