#include "ui/SettingsDialog.h"
#include "services/AuthService.h"
#include "core/DatabaseManager.h"
#include "core/ThemeManager.h"
#include "core/Types.h"
#include "resources/styles.h"
#include "resources/utf8.h"

#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <sqlite3.h>

namespace remont {

namespace {

class CircleAvatar : public wxPanel {
public:
    CircleAvatar(wxWindow* parent, const wxString& text,
                 const wxColour& bg, const wxColour& fg, int size)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(size, size)),
          text_(text), bg_(bg), fg_(fg), size_(size)
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetBackgroundColour(parent->GetBackgroundColour());
        SetMinSize(wxSize(size, size));
        Bind(wxEVT_PAINT, &CircleAvatar::onPaint, this);
    }

private:
    void onPaint(wxPaintEvent&) {
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
        dc.Clear();

        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        if (!gc) return;

        gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
        gc->SetBrush(wxBrush(bg_));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawEllipse(0, 0, size_, size_);

        gc->SetFont(wxFont(16, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                           wxFONTWEIGHT_BOLD), fg_);
        double tw, th;
        gc->GetTextExtent(text_, &tw, &th);
        gc->DrawText(text_, (size_ - tw) / 2.0, (size_ - th) / 2.0);
    }

    wxString text_;
    wxColour bg_;
    wxColour fg_;
    int size_;
};

wxPanel* makeChoiceCard(wxWindow* parent, const wxString& title, const wxString& subtitle,
                        bool active)
{
    auto& tm = ThemeManager::instance();

    auto* panel = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 80));
    wxColour cardBg = active ? wxColour(0xEF, 0xF6, 0xFF) : tm.surface();
    panel->SetBackgroundColour(cardBg);

    auto* sizer = new wxBoxSizer(wxVERTICAL);

    auto* titleLabel = new wxStaticText(panel, wxID_ANY, title);
    titleLabel->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    titleLabel->SetForegroundColour(active ? tm.primary() : tm.text());
    titleLabel->SetBackgroundColour(cardBg);

    auto* subtitleLabel = new wxStaticText(panel, wxID_ANY, subtitle);
    subtitleLabel->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    subtitleLabel->SetForegroundColour(tm.muted());
    subtitleLabel->SetBackgroundColour(cardBg);

    sizer->AddStretchSpacer();
    sizer->Add(titleLabel, 0, wxALIGN_CENTER);
    sizer->AddSpacer(4);
    sizer->Add(subtitleLabel, 0, wxALIGN_CENTER);
    sizer->AddStretchSpacer();

    panel->SetSizer(sizer);
    panel->SetCursor(wxCursor(wxCURSOR_HAND));
    titleLabel->SetCursor(wxCursor(wxCURSOR_HAND));
    subtitleLabel->SetCursor(wxCursor(wxCURSOR_HAND));

    titleLabel->Bind(wxEVT_LEFT_UP, [panel](wxMouseEvent& e) {
        wxMouseEvent copy(e);
        copy.SetEventObject(panel);
        panel->GetEventHandler()->ProcessEvent(copy);
    });
    subtitleLabel->Bind(wxEVT_LEFT_UP, [panel](wxMouseEvent& e) {
        wxMouseEvent copy(e);
        copy.SetEventObject(panel);
        panel->GetEventHandler()->ProcessEvent(copy);
    });

    return panel;
}

}

SettingsDialog::SettingsDialog(wxWindow* parent, const User& user)
    : wxDialog(parent, wxID_ANY, utf8::U("Настройки"),
               wxDefaultPosition, wxSize(900, 700),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      user_(user)
{
    auto& tm = ThemeManager::instance();

    SetBackgroundColour(tm.background());

    auto* rootSizer = new wxBoxSizer(wxVERTICAL);

    scroll_ = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                   wxVSCROLL | wxBORDER_NONE);
    scroll_->SetBackgroundColour(tm.background());
    scroll_->SetScrollRate(0, 16);

    auto* root = new wxBoxSizer(wxVERTICAL);
    root->AddSpacer(24);

    auto* title = new wxStaticText(scroll_, wxID_ANY, utf8::U("Настройки"));
    title->SetFont(wxFont(22, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    title->SetForegroundColour(tm.text());
    title->SetBackgroundColour(tm.background());
    root->Add(title, 0, wxLEFT | wxRIGHT, 32);
    root->AddSpacer(20);

    root->Add(buildAccountCard(scroll_), 0, wxEXPAND | wxLEFT | wxRIGHT, 32);
    root->AddSpacer(16);
    root->Add(buildAppearanceCard(scroll_), 0, wxEXPAND | wxLEFT | wxRIGHT, 32);
    root->AddSpacer(16);
    root->Add(buildPasswordCard(scroll_), 0, wxEXPAND | wxLEFT | wxRIGHT, 32);
    root->AddSpacer(24);

    scroll_->SetSizer(root);
    scroll_->FitInside();

    rootSizer->Add(scroll_, 1, wxEXPAND);
    SetSizer(rootSizer);

    CentreOnScreen();
}

wxPanel* SettingsDialog::buildAccountCard(wxWindow* parent) {
    auto& tm = ThemeManager::instance();

    auto* card = new wxPanel(parent, wxID_ANY);
    card->SetBackgroundColour(tm.surface());

    auto* sizer = new wxBoxSizer(wxVERTICAL);

    auto* header = new wxStaticText(card, wxID_ANY, utf8::U("Данные аккаунта"));
    header->SetFont(wxFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    header->SetForegroundColour(tm.text());
    header->SetBackgroundColour(tm.surface());
    sizer->Add(header, 0, wxALL, 20);

    auto* sep1 = new wxPanel(card, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
    sep1->SetBackgroundColour(tm.border());
    sizer->Add(sep1, 0, wxEXPAND);

    auto* avatarRow = new wxBoxSizer(wxHORIZONTAL);

    auto* avatar = new CircleAvatar(card, "AD",
                                    wxColour(0xDB, 0xEA, 0xFE),
                                    wxColour(0x1E, 0x40, 0xAF), 56);

    auto* nameCol = new wxBoxSizer(wxVERTICAL);
    auto* nameLabel = new wxStaticText(card, wxID_ANY, utf8::U(user_.login.c_str()));
    nameLabel->SetFont(wxFont(15, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    nameLabel->SetForegroundColour(tm.text());
    nameLabel->SetBackgroundColour(tm.surface());
    auto* roleLabel = new wxStaticText(card, wxID_ANY, utf8::U("Администратор"));
    roleLabel->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    roleLabel->SetForegroundColour(tm.muted());
    roleLabel->SetBackgroundColour(tm.surface());
    nameCol->Add(nameLabel);
    nameCol->Add(roleLabel);

    avatarRow->Add(avatar, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 20);
    avatarRow->AddSpacer(12);
    avatarRow->Add(nameCol, 1, wxALIGN_CENTER_VERTICAL);
    sizer->Add(avatarRow, 0, wxEXPAND | wxTOP | wxBOTTOM, 16);

    auto* sep2 = new wxPanel(card, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
    sep2->SetBackgroundColour(tm.border());
    sizer->Add(sep2, 0, wxEXPAND);

    auto makeRow = [&](const wxString& label, const wxString& value) {
        auto* row = new wxBoxSizer(wxHORIZONTAL);
        auto* lbl = new wxStaticText(card, wxID_ANY, label);
        lbl->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        lbl->SetForegroundColour(tm.muted());
        lbl->SetBackgroundColour(tm.surface());
        auto* val = new wxStaticText(card, wxID_ANY, value);
        val->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        val->SetForegroundColour(tm.text());
        val->SetBackgroundColour(tm.surface());
        row->Add(lbl, 1, wxALIGN_CENTER_VERTICAL);
        row->Add(val, 0, wxALIGN_CENTER_VERTICAL);
        return row;
    };

    sizer->Add(makeRow(utf8::U("Логин"), utf8::U(user_.login.c_str())),
               0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 20);
    sizer->AddSpacer(10);
    sizer->Add(makeRow(utf8::U("Роль"), utf8::U("Администратор")),
               0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    sizer->AddSpacer(10);
    sizer->Add(makeRow(utf8::U("Статус"), utf8::U("Активен")),
               0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    sizer->AddSpacer(10);
    sizer->Add(makeRow(utf8::U("Последний вход"), utf8::U("30.09.2026 08:45")),
               0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 20);

    card->SetSizer(sizer);
    return card;
}

wxPanel* SettingsDialog::buildAppearanceCard(wxWindow* parent) {
    auto& tm = ThemeManager::instance();

    auto* card = new wxPanel(parent, wxID_ANY);
    card->SetBackgroundColour(tm.surface());

    auto* sizer = new wxBoxSizer(wxVERTICAL);

    auto* header = new wxStaticText(card, wxID_ANY, utf8::U("Оформление"));
    header->SetFont(wxFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    header->SetForegroundColour(tm.text());
    header->SetBackgroundColour(tm.surface());
    sizer->Add(header, 0, wxALL, 20);

    auto* sep = new wxPanel(card, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
    sep->SetBackgroundColour(tm.border());
    sizer->Add(sep, 0, wxEXPAND);

    bool lightActive = (tm.theme() == Theme::Light);
    bool darkActive  = (tm.theme() == Theme::Dark);
    bool sysActive   = (tm.theme() == Theme::System);

    bool smallActive  = (tm.textSize() == TextSize::Small);
    bool normalActive = (tm.textSize() == TextSize::Normal);
    bool largeActive  = (tm.textSize() == TextSize::Large);

    auto* themeLabel = new wxStaticText(card, wxID_ANY, utf8::U("Тема"));
    themeLabel->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    themeLabel->SetForegroundColour(tm.muted());
    themeLabel->SetBackgroundColour(tm.surface());
    sizer->Add(themeLabel, 0, wxLEFT | wxRIGHT | wxTOP, 20);
    sizer->AddSpacer(8);

    auto* themeRow = new wxBoxSizer(wxHORIZONTAL);
    themePanels_[0] = makeChoiceCard(card, utf8::U("Светлая"), utf8::U(""), lightActive);
    themePanels_[1] = makeChoiceCard(card, utf8::U("Тёмная"), utf8::U(""), darkActive);
    themePanels_[2] = makeChoiceCard(card, utf8::U("Как в системе"), utf8::U(""), sysActive);

    themeRow->Add(themePanels_[0], 1, wxEXPAND);
    themeRow->AddSpacer(12);
    themeRow->Add(themePanels_[1], 1, wxEXPAND);
    themeRow->AddSpacer(12);
    themeRow->Add(themePanels_[2], 1, wxEXPAND);
    sizer->Add(themeRow, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);

    auto* sizeLabel = new wxStaticText(card, wxID_ANY, utf8::U("Размер текста"));
    sizeLabel->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    sizeLabel->SetForegroundColour(tm.muted());
    sizeLabel->SetBackgroundColour(tm.surface());
    sizer->Add(sizeLabel, 0, wxLEFT | wxRIGHT | wxTOP, 20);
    sizer->AddSpacer(8);

    auto* sizeRow = new wxBoxSizer(wxHORIZONTAL);
    textSizePanels_[0] = makeChoiceCard(card, utf8::U("Аа"), utf8::U("Мелкий"), smallActive);
    textSizePanels_[1] = makeChoiceCard(card, utf8::U("Аа"), utf8::U("Обычный"), normalActive);
    textSizePanels_[2] = makeChoiceCard(card, utf8::U("Аа"), utf8::U("Крупный"), largeActive);

    sizeRow->Add(textSizePanels_[0], 1, wxEXPAND);
    sizeRow->AddSpacer(12);
    sizeRow->Add(textSizePanels_[1], 1, wxEXPAND);
    sizeRow->AddSpacer(12);
    sizeRow->Add(textSizePanels_[2], 1, wxEXPAND);
    sizer->Add(sizeRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 20);

    for (int i = 0; i < 3; ++i) {
        themePanels_[i]->Bind(wxEVT_LEFT_UP,
            [this, i](wxMouseEvent& e) { onThemeClick(e, i); });
        textSizePanels_[i]->Bind(wxEVT_LEFT_UP,
            [this, i](wxMouseEvent& e) { onTextSizeClick(e, i); });
    }

    card->SetSizer(sizer);
    return card;
}

wxPanel* SettingsDialog::buildPasswordCard(wxWindow* parent) {
    auto& tm = ThemeManager::instance();

    auto* card = new wxPanel(parent, wxID_ANY);
    card->SetBackgroundColour(tm.surface());

    auto* sizer = new wxBoxSizer(wxVERTICAL);

    auto* header = new wxStaticText(card, wxID_ANY, utf8::U("Смена пароля"));
    header->SetFont(wxFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    header->SetForegroundColour(tm.text());
    header->SetBackgroundColour(tm.surface());
    sizer->Add(header, 0, wxALL, 20);

    auto* sep = new wxPanel(card, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
    sep->SetBackgroundColour(tm.border());
    sizer->Add(sep, 0, wxEXPAND);

    auto makeField = [&](const wxString& label, wxTextCtrl*& field) {
        auto* lbl = new wxStaticText(card, wxID_ANY, label);
        lbl->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        lbl->SetForegroundColour(tm.muted());
        lbl->SetBackgroundColour(tm.surface());
        sizer->Add(lbl, 0, wxLEFT | wxRIGHT | wxTOP, 20);
        sizer->AddSpacer(6);
        field = new wxTextCtrl(card, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 38),
                               wxTE_PASSWORD | wxBORDER_SIMPLE);
        field->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        field->SetBackgroundColour(tm.surface());
        field->SetForegroundColour(tm.text());
        sizer->Add(field, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    };

    makeField(utf8::U("Текущий пароль"), currentPassField_);
    makeField(utf8::U("Новый пароль"), newPassField_);
    makeField(utf8::U("Повторите новый пароль"), repeatPassField_);

    messageLabel_ = new wxStaticText(card, wxID_ANY, "");
    messageLabel_->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    messageLabel_->SetBackgroundColour(tm.surface());
    sizer->Add(messageLabel_, 0, wxLEFT | wxRIGHT | wxTOP, 20);

    auto* btn = new wxButton(card, wxID_ANY, utf8::U("Изменить пароль"),
                             wxDefaultPosition, wxSize(200, 40), wxBORDER_NONE);
    btn->SetBackgroundColour(tm.primary());
    btn->SetForegroundColour(*wxWHITE);
    btn->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    btn->Bind(wxEVT_BUTTON, &SettingsDialog::onChangePassword, this);
    sizer->Add(btn, 0, wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 20);

    card->SetSizer(sizer);
    return card;
}

void SettingsDialog::onThemeClick(wxMouseEvent&, int themeIndex) {
    Theme newTheme = Theme::Light;
    if (themeIndex == 1) newTheme = Theme::Dark;
    else if (themeIndex == 2) newTheme = Theme::System;

    ThemeManager::instance().setTheme(newTheme);

    EndModal(wxID_RETRY);
}

void SettingsDialog::onTextSizeClick(wxMouseEvent&, int sizeIndex) {
    TextSize newSize = TextSize::Normal;
    if (sizeIndex == 0) newSize = TextSize::Small;
    else if (sizeIndex == 2) newSize = TextSize::Large;

    ThemeManager::instance().setTextSize(newSize);

    EndModal(wxID_RETRY);
}

void SettingsDialog::onChangePassword(wxCommandEvent&) {
    auto& tm = ThemeManager::instance();

    wxString current = currentPassField_->GetValue();
    wxString next = newPassField_->GetValue();
    wxString repeat = repeatPassField_->GetValue();

    if (current.IsEmpty() || next.IsEmpty() || repeat.IsEmpty()) {
        messageLabel_->SetForegroundColour(tm.danger());
        messageLabel_->SetLabel(utf8::U("Заполните все поля"));
        return;
    }
    if (next.length() < 4) {
        messageLabel_->SetForegroundColour(tm.danger());
        messageLabel_->SetLabel(utf8::U("Пароль должен быть не короче 4 символов"));
        return;
    }
    if (next != repeat) {
        messageLabel_->SetForegroundColour(tm.danger());
        messageLabel_->SetLabel(utf8::U("Пароли не совпадают"));
        return;
    }

    std::string currentUtf8 = std::string(current.ToUTF8().data());
    std::string currentHash = AuthService::hashPassword(currentUtf8);

    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) {
        messageLabel_->SetForegroundColour(tm.danger());
        messageLabel_->SetLabel(utf8::U("Ошибка БД"));
        return;
    }

    const char* checkSql = "SELECT password_hash FROM users WHERE login = ? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, checkSql, -1, &stmt, nullptr) != SQLITE_OK) {
        messageLabel_->SetForegroundColour(tm.danger());
        messageLabel_->SetLabel(utf8::U("Ошибка запроса"));
        return;
    }
    sqlite3_bind_text(stmt, 1, user_.login.c_str(), -1, SQLITE_TRANSIENT);

    std::string dbHash;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* h = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        dbHash = h ? h : "";
    }
    sqlite3_finalize(stmt);

    if (dbHash != currentHash) {
        messageLabel_->SetForegroundColour(tm.danger());
        messageLabel_->SetLabel(utf8::U("Текущий пароль неверный"));
        return;
    }

    std::string nextUtf8 = std::string(next.ToUTF8().data());
    std::string nextHash = AuthService::hashPassword(nextUtf8);

    const char* updateSql = "UPDATE users SET password_hash = ? WHERE login = ?;";
    if (sqlite3_prepare_v2(db, updateSql, -1, &stmt, nullptr) != SQLITE_OK) {
        messageLabel_->SetForegroundColour(tm.danger());
        messageLabel_->SetLabel(utf8::U("Ошибка запроса"));
        return;
    }
    sqlite3_bind_text(stmt, 1, nextHash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, user_.login.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);

    if (ok) {
        messageLabel_->SetForegroundColour(tm.success());
        messageLabel_->SetLabel(utf8::U("Пароль изменён"));
        currentPassField_->Clear();
        newPassField_->Clear();
        repeatPassField_->Clear();
    } else {
        messageLabel_->SetForegroundColour(tm.danger());
        messageLabel_->SetLabel(utf8::U("Не удалось изменить пароль"));
    }
}

}