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