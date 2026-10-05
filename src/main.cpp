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
#include <iostream>

using namespace remont;

namespace fs = std::filesystem;

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

        std::string adminHash = remont::AuthService::hashPassword("admin");
        std::cout << "[MAIN] hashPassword(\"admin\") = " << adminHash << std::endl;

        {
            const char* checkSql = "SELECT COUNT(*) FROM users WHERE login = 'admin';";
            sqlite3_stmt* stmt = nullptr;
            sqlite3_prepare_v2(db.handle(), checkSql, -1, &stmt, nullptr);
            int count = 0;
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                count = sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);

            std::cout << "[MAIN] admin в БД найден: " << count << " раз" << std::endl;

            if (count == 0) {
                std::string insertSql =
                    "INSERT INTO users(login, password_hash, role, is_active) "
                    "VALUES('admin', '" + adminHash + "', 'admin', 1);";
                db.execute(insertSql);
                std::cout << "[MAIN] admin создан" << std::endl;
            }
        }

        remont::LoginDialog dlg(nullptr);
        if (dlg.ShowModal() != wxID_OK) {
            return false;
        }

        remont::User user = dlg.getUser();
        std::cout << "[MAIN] Вошёл пользователь: '" << user.login
                  << "' роль=" << static_cast<int>(user.role) << std::endl;

        auto* mainWindow = new remont::MainWindow(user);
        mainWindow->Show();

        return true;
    }
};

wxIMPLEMENT_APP(RemontApp);