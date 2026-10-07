#include "core/DatabaseManager.h"
#include "core/ConfigManager.h"
#include "core/ThemeManager.h"
#include "services/AuthService.h"
#include "ui/LoginDialog.h"
#include "ui/MainWindow.h"
#include "resources/utf8.h"

#include <wx/wx.h>
#include <sqlite3.h>
#include <hpdf.h>

#include <filesystem>
#include <windows.h>
#include <iostream>

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
        "VALUES('#1', 1, 1, 1, 'В ремонте', 'Не включается, не заряжается аккумулятор.', 3500);",

        "INSERT INTO orders(order_number, client_id, device_id, user_id, status, description, total_cost) "
        "VALUES('#2', 2, 2, 1, 'Принят', 'Разбит экран, не реагирует на касания.', 0);",

        "INSERT INTO orders(order_number, client_id, device_id, user_id, status, description, total_cost) "
        "VALUES('#3', 3, 3, 1, 'Готов', 'Не держит заряд, быстро разряжается.', 2800);",

        "INSERT INTO orders(order_number, client_id, device_id, user_id, status, description, total_cost) "
        "VALUES('#4', 4, 4, 1, 'Диагностика', 'Нет изображения, подсветка работает.', 0);",

        "INSERT INTO orders(order_number, client_id, device_id, user_id, status, description, total_cost) "
        "VALUES('#5', 5, 5, 1, 'Ожидает запчасть', 'Замятие бумаги, требуется замена ролика.', 1500);",

        "INSERT INTO orders(order_number, client_id, device_id, user_id, status, description, total_cost) "
        "VALUES('#6', 6, 6, 1, 'Выдан', 'Замена клавиатуры, отремонтирован.', 4200);"
    };
    for (auto sql : orders) db.execute(sql);

    const char* parts[] = {
        "INSERT INTO parts(name, article, quantity, price, min_quantity) VALUES('Матрица 15.6 FHD IPS', 'A001', 5, 4500, 3);",
        "INSERT INTO parts(name, article, quantity, price, min_quantity) VALUES('Матрица 13.3 FHD IPS', 'A002', 2, 5200, 3);",
        "INSERT INTO parts(name, article, quantity, price, min_quantity) VALUES('АКБ для Xiaomi Redmi Note 11', 'B001', 8, 1200, 4);",
        "INSERT INTO parts(name, article, quantity, price, min_quantity) VALUES('АКБ для Samsung Galaxy A52', 'B002', 1, 1500, 4);",
        "INSERT INTO parts(name, article, quantity, price, min_quantity) VALUES('Термопаста Arctic MX-4 (4г)', 'C001', 15, 320, 5);",
        "INSERT INTO parts(name, article, quantity, price, min_quantity) VALUES('Термопаста Halnziye HY-883', 'C002', 3, 180, 5);",
        "INSERT INTO parts(name, article, quantity, price, min_quantity) VALUES('Разъём зарядки Type-C', 'D001', 12, 250, 5);",
        "INSERT INTO parts(name, article, quantity, price, min_quantity) VALUES('Разъём зарядки micro-USB', 'D002', 6, 120, 5);",
        "INSERT INTO parts(name, article, quantity, price, min_quantity) VALUES('Кулер для ноутбука HP Pavilion', 'E001', 2, 1800, 2);"
    };
    for (auto sql : parts) db.execute(sql);
}

class RemontApp : public wxApp {
public:
    bool OnInit() override;
    void showLogin();
    void openMainWindow(const remont::User& user);

private:
    wxFrame* holderFrame_ = nullptr;
};

bool RemontApp::OnInit() {
    SetConsoleOutputCP(CP_UTF8);
    wxInitAllImageHandlers();

    std::cerr << "[APP] OnInit start\n";
    remont::ThemeManager::instance().load();
    std::cerr << "[APP] ThemeManager loaded\n";

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

    holderFrame_ = new wxFrame(nullptr, wxID_ANY, "holder",
                               wxDefaultPosition, wxSize(1, 1));
    holderFrame_->Hide();

    CallAfter([this]() { showLogin(); });
    return true;
}

void RemontApp::showLogin() {
    remont::LoginDialog dlg(holderFrame_);
    if (dlg.ShowModal() != wxID_OK) {
        ExitMainLoop();
        return;
    }

    remont::User user = dlg.getUser();
    openMainWindow(user);
}

void RemontApp::openMainWindow(const remont::User& user) {
    std::cerr << "[APP] openMainWindow, dark="
              << (remont::ThemeManager::instance().isDark() ? "yes" : "no") << "\n";

    auto* mainWindow = new remont::MainWindow(user);

    mainWindow->Bind(wxEVT_CLOSE_WINDOW,
                     [this, mainWindow, user](wxCloseEvent& e) {
        bool logout = mainWindow->logoutRequested();
        bool themeChanged = mainWindow->themeChanged();

        e.Skip();
        mainWindow->Destroy();

        if (themeChanged) {
            CallAfter([this, user]() { openMainWindow(user); });
        } else if (logout) {
            CallAfter([this]() { showLogin(); });
        } else {
            ExitMainLoop();
        }
    });

    mainWindow->Show();
}

wxIMPLEMENT_APP(RemontApp);