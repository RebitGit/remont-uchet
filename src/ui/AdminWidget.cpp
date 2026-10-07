#include "ui/AdminWidget.h"
#include "services/AuthService.h"
#include "core/DatabaseManager.h"
#include "core/ConfigManager.h"
#include "core/ThemeManager.h"
#include "core/Logger.h"
#include "core/Types.h"
#include "resources/styles.h"
#include "resources/utf8.h"

#include <sqlite3.h>
#include <filesystem>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace fs = std::filesystem;

namespace remont {

AdminWidget::AdminWidget(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
{
    auto& tm = ThemeManager::instance();
    SetBackgroundColour(tm.background());

    mainSizer_ = new wxBoxSizer(wxVERTICAL);
    mainSizer_->AddSpacer(20);

    buildToolbar(mainSizer_);
    mainSizer_->AddSpacer(12);
    buildTable(mainSizer_);
    buildFooter(mainSizer_);
    mainSizer_->AddSpacer(20);

    SetSizer(mainSizer_);
    loadUsers();
}

void AdminWidget::buildToolbar(wxSizer* root) {
    auto& tm = ThemeManager::instance();

    auto* bar = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 64));
    bar->SetBackgroundColour(tm.surface());

    auto* sizer = new wxBoxSizer(wxHORIZONTAL);

    auto* title = new wxStaticText(bar, wxID_ANY, utf8::U("Пользователи системы"));
    title->SetFont(wxFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    title->SetForegroundColour(tm.text());
    title->SetBackgroundColour(tm.surface());

    auto* backupBtn = new wxButton(bar, wxID_ANY, utf8::U("Резервная копия"),
                                   wxDefaultPosition, wxSize(180, 40), wxBORDER_NONE);
    backupBtn->SetBackgroundColour(tm.surface());
    backupBtn->SetForegroundColour(tm.text());
    backupBtn->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    backupBtn->Bind(wxEVT_BUTTON, &AdminWidget::onBackup, this);

    auto* addBtn = new wxButton(bar, wxID_ANY, utf8::U("+ Добавить пользователя"),
                                wxDefaultPosition, wxSize(240, 40), wxBORDER_NONE);
    addBtn->SetBackgroundColour(tm.primary());
    addBtn->SetForegroundColour(*wxWHITE);
    addBtn->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    addBtn->Bind(wxEVT_BUTTON, &AdminWidget::onAddUser, this);

    sizer->Add(title, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 16);
    sizer->AddStretchSpacer(1);
    sizer->Add(backupBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    sizer->Add(addBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);

    bar->SetSizer(sizer);
    root->Add(bar, 0, wxEXPAND | wxLEFT | wxRIGHT, 24);
}

void AdminWidget::buildTable(wxSizer* root) {
    auto& tm = ThemeManager::instance();

    table_ = new ClickableListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 400),
                                   wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_NONE);
    table_->SetBackgroundColour(tm.surface());
    table_->SetForegroundColour(tm.text());
    table_->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

    table_->AppendColumn(utf8::U("ЛОГИН"),   wxLIST_FORMAT_LEFT, 200);
    table_->AppendColumn(utf8::U("РОЛЬ"),    wxLIST_FORMAT_LEFT, 200);
    table_->AppendColumn(utf8::U("СТАТУС"),  wxLIST_FORMAT_LEFT, 150);
    table_->AppendColumn(utf8::U("СБРОС"),   wxLIST_FORMAT_CENTER, 100);
    table_->AppendColumn(utf8::U("РОЛЬ"),    wxLIST_FORMAT_CENTER, 100);
    table_->AppendColumn(utf8::U("ВКЛ/ВЫКЛ"),wxLIST_FORMAT_CENTER, 120);

    table_->setOnLeftClick([this](int row, int col) {
        onTableClick(row, col);
    });

    root->Add(table_, 1, wxEXPAND | wxLEFT | wxRIGHT, 24);
}

void AdminWidget::buildFooter(wxSizer* root) {
    auto& tm = ThemeManager::instance();

    auto* footer = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 40));
    footer->SetBackgroundColour(tm.surface());

    footerCount_ = new wxStaticText(footer, wxID_ANY, "");
    footerCount_->SetForegroundColour(tm.muted());
    footerCount_->SetBackgroundColour(tm.surface());
    footerCount_->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

    auto* sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(footerCount_, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 16);
    footer->SetSizer(sizer);
    root->Add(footer, 0, wxEXPAND | wxLEFT | wxRIGHT, 24);
}

void AdminWidget::loadUsers() {
    if (!table_) return;
    table_->DeleteAllItems();

    allUsers_.clear();

    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return;

    const char* sql = "SELECT id, login, password_hash, role, is_active FROM users ORDER BY id;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        User u;
        u.id = sqlite3_column_int(stmt, 0);
        const char* l = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        u.login = l ? l : "";
        const char* h = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        u.passwordHash = h ? h : "";
        const char* r = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        std::string roleStr = r ? r : "";
        if (roleStr == "admin") u.role = Role::Admin;
        else if (roleStr == "master") u.role = Role::Master;
        else if (roleStr == "warehouse") u.role = Role::Warehouse;
        else u.role = Role::Operator;
        u.isActive = sqlite3_column_int(stmt, 4) == 1;
        allUsers_.push_back(u);
    }
    sqlite3_finalize(stmt);

    applyFilter();
}

void AdminWidget::applyFilter() {
    if (!table_) return;

    auto& tm = ThemeManager::instance();
    table_->DeleteAllItems();

    long idx = 0;
    for (auto& u : allUsers_) {
        wxString roleStr;
        if (u.role == Role::Admin) roleStr = utf8::U("Администратор");
        else if (u.role == Role::Master) roleStr = utf8::U("Мастер");
        else if (u.role == Role::Warehouse) roleStr = utf8::U("Кладовщик");
        else roleStr = utf8::U("Оператор");

        long row = table_->InsertItem(idx, utf8::U(u.login.c_str()));
        table_->SetItem(row, 1, roleStr);
        table_->SetItem(row, 2, u.isActive ? utf8::U("Активен") : utf8::U("Отключён"));
        table_->SetItem(row, 3, utf8::U("🔑"));
        table_->SetItem(row, 4, utf8::U("✎"));
        table_->SetItem(row, 5, u.isActive ? utf8::U("⏻") : utf8::U("▶"));

        table_->SetItemData(row, u.id);

        if (!u.isActive) {
            table_->SetItemTextColour(row, wxColour(0x9C, 0xA3, 0xAF));
        } else {
            table_->SetItemTextColour(row, tm.text());
        }

        idx++;
    }

    if (footerCount_) {
        footerCount_->SetLabel(utf8::U("Пользователей: ") +
                               wxString::Format("%zu", allUsers_.size()));
    }
}

void AdminWidget::onTableClick(int row, int col) {
    if (row < 0 || col < 0) return;

    long userId = table_->GetItemData(row);
    if (userId == 0) return;

    if (col == 3) {
        resetPassword(static_cast<int>(userId));
    } else if (col == 4) {
        changeRole(static_cast<int>(userId));
    } else if (col == 5) {
        toggleActive(static_cast<int>(userId));
    }
}

void AdminWidget::onAddUser(wxCommandEvent&) {
    wxString login = wxGetTextFromUser(utf8::U("Логин:"), utf8::U("Добавить пользователя"),
                                       "", this);
    if (login.IsEmpty()) return;

    wxString password = wxGetTextFromUser(utf8::U("Пароль:"), utf8::U("Добавить пользователя"),
                                          "", this);
    if (password.IsEmpty()) return;

    std::string loginUtf8 = std::string(login.ToUTF8().data());
    std::string passUtf8 = std::string(password.ToUTF8().data());

    std::string hash = AuthService::hashPassword(passUtf8);

    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return;

    const char* sql = "INSERT INTO users(login, password_hash, role, is_active) VALUES(?, ?, 'operator', 1);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return;

    sqlite3_bind_text(stmt, 1, loginUtf8.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, hash.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        wxMessageBox(utf8::U("Не удалось добавить пользователя"),
                     utf8::U("Ошибка"), wxOK | wxICON_ERROR, this);
        return;
    }
    sqlite3_finalize(stmt);

    Logger::instance().log(0, "Добавлен пользователь: " + loginUtf8, "user", 0);

    loadUsers();
}

void AdminWidget::toggleActive(int userId) {
    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return;

    const char* sql = "UPDATE users SET is_active = CASE is_active WHEN 1 THEN 0 ELSE 1 END WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return;
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    Logger::instance().log(0, "Изменён статус пользователя id=" + std::to_string(userId),
                           "user", userId);

    loadUsers();
}

void AdminWidget::changeRole(int userId) {
    wxArrayString choices;
    choices.Add(utf8::U("Оператор"));
    choices.Add(utf8::U("Мастер"));
    choices.Add(utf8::U("Кладовщик"));
    choices.Add(utf8::U("Администратор"));

    wxString selected = wxGetSingleChoice(utf8::U("Выберите новую роль:"),
                                          utf8::U("Смена роли"),
                                          choices, this);
    if (selected.IsEmpty()) return;

    std::string roleStr;
    if (selected == utf8::U("Администратор")) roleStr = "admin";
    else if (selected == utf8::U("Мастер")) roleStr = "master";
    else if (selected == utf8::U("Кладовщик")) roleStr = "warehouse";
    else roleStr = "operator";

    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return;

    const char* sql = "UPDATE users SET role = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return;
    sqlite3_bind_text(stmt, 1, roleStr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, userId);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    Logger::instance().log(0, "Изменена роль пользователя id=" + std::to_string(userId) +
                               " на " + roleStr, "user", userId);

    loadUsers();
}

void AdminWidget::resetPassword(int userId) {
    wxString password = wxGetTextFromUser(utf8::U("Новый пароль:"),
                                          utf8::U("Сброс пароля"), "", this);
    if (password.IsEmpty()) return;

    std::string passUtf8 = std::string(password.ToUTF8().data());
    std::string hash = AuthService::hashPassword(passUtf8);

    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return;

    const char* sql = "UPDATE users SET password_hash = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return;
    sqlite3_bind_text(stmt, 1, hash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, userId);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    Logger::instance().log(0, "Сброшен пароль пользователя id=" + std::to_string(userId),
                           "user", userId);

    wxMessageBox(utf8::U("Пароль изменён"),
                 utf8::U("Готово"), wxOK | wxICON_INFORMATION, this);
}

void AdminWidget::onBackup(wxCommandEvent&) {
    auto& cfg = ConfigManager::instance();

    fs::path dbPath = cfg.dbPath();
    fs::path backupDir = dbPath.parent_path() / ".." / "backups";
    fs::create_directories(backupDir);

    std::time_t t = std::time(nullptr);
    std::tm* tm = std::localtime(&t);
    char nameBuf[64];
    std::strftime(nameBuf, sizeof(nameBuf), "backup_%Y-%m-%d_%H-%M-%S.db", tm);

    fs::path outPath = backupDir / nameBuf;

    try {
        fs::copy_file(dbPath, outPath, fs::copy_options::overwrite_existing);

        Logger::instance().log(0, "Создана резервная копия: " + std::string(nameBuf),
                               "db", 0);

        wxString msg = utf8::U("Резервная копия создана:\n") +
                       utf8::U(outPath.string().c_str());
        wxMessageBox(msg, utf8::U("Резервное копирование"),
                     wxOK | wxICON_INFORMATION, this);
    } catch (const std::exception& e) {
        wxMessageBox(utf8::U("Не удалось создать резервную копию"),
                     utf8::U("Ошибка"), wxOK | wxICON_ERROR, this);
    }
}

}