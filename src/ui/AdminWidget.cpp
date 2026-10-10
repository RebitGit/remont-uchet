#include "ui/AdminWidget.h"
#include "services/AuthService.h"
#include "core/DatabaseManager.h"
#include "core/ConfigManager.h"
#include "core/ThemeManager.h"
#include "core/Logger.h"
#include "core/Types.h"
#include "resources/styles.h"
#include "resources/utf8.h"

#include <wx/dcbuffer.h>
#include <wx/dcmemory.h>
#include <wx/graphics.h>
#include <wx/statbmp.h>
#include <wx/menu.h>
#include <wx/artprov.h>
#include <sqlite3.h>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace fs = std::filesystem;

namespace remont {

namespace {

const wxString ICON_EDIT_ROLE = "resources/icons/square-pen.png";
const wxString ICON_KEY_PASS  = "resources/icons/key-round.png";
const wxString ICON_DELETE    = "resources/icons/trash.png";

class UserAvatar : public wxPanel {
public:
    UserAvatar(wxWindow* parent, const wxString& letter,
               const wxColour& bg, const wxColour& fg, int size)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(size, size)),
          letter_(letter), bg_(bg), fg_(fg), size_(size)
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetBackgroundColour(parent->GetBackgroundColour());
        SetMinSize(wxSize(size, size));
        Bind(wxEVT_PAINT, &UserAvatar::onPaint, this);
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

        gc->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                           wxFONTWEIGHT_BOLD), fg_);
        double tw, th;
        gc->GetTextExtent(letter_, &tw, &th);
        gc->DrawText(letter_, (size_ - tw) / 2.0, (size_ - th) / 2.0);
    }

    wxString letter_;
    wxColour bg_;
    wxColour fg_;
    int size_;
};

class Badge : public wxPanel {
public:
    Badge(wxWindow* parent, const wxString& text,
          const wxColour& bg, const wxColour& fg)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 22)),
          text_(text), bg_(bg), fg_(fg)
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetBackgroundColour(parent->GetBackgroundColour());
        wxClientDC dc(this);
        dc.SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        wxSize sz = dc.GetTextExtent(text);
        SetMinSize(wxSize(sz.x + 20, 22));
        Bind(wxEVT_PAINT, &Badge::onPaint, this);
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
        wxSize s = GetSize();
        gc->DrawRoundedRectangle(0, 0, s.GetWidth(), s.GetHeight(), 11);

        gc->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                           wxFONTWEIGHT_BOLD), fg_);
        double tw, th;
        gc->GetTextExtent(text_, &tw, &th);
        gc->DrawText(text_, (s.GetWidth() - tw) / 2.0,
                     (s.GetHeight() - th) / 2.0);
    }

    wxString text_;
    wxColour bg_;
    wxColour fg_;
};

class StatusDot : public wxPanel {
public:
    StatusDot(wxWindow* parent, const wxColour& color, int size = 8)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(size, size)),
          color_(color), size_(size)
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetBackgroundColour(parent->GetBackgroundColour());
        SetMinSize(wxSize(size, size));
        Bind(wxEVT_PAINT, &StatusDot::onPaint, this);
    }

private:
    void onPaint(wxPaintEvent&) {
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
        dc.Clear();

        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        if (!gc) return;
        gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
        gc->SetBrush(wxBrush(color_));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawEllipse(0, 0, size_, size_);
    }

    wxColour color_;
    int size_;
};

class ToggleSwitch : public wxPanel {
public:
    ToggleSwitch(wxWindow* parent, bool initial = true)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(46, 24)),
          on_(initial)
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetBackgroundColour(parent->GetBackgroundColour());
        SetMinSize(wxSize(46, 24));
        SetCursor(wxCursor(wxCURSOR_HAND));
        Bind(wxEVT_PAINT, &ToggleSwitch::onPaint, this);
        Bind(wxEVT_LEFT_UP, [this](wxMouseEvent&) {
            on_ = !on_;
            Refresh();
            if (onChange_) onChange_(on_);
        });
    }

    void setOnChange(std::function<void(bool)> cb) { onChange_ = std::move(cb); }
    bool isOn() const { return on_; }

private:
    void onPaint(wxPaintEvent&) {
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
        dc.Clear();

        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        if (!gc) return;
        gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);

        int W = GetSize().GetWidth();
        int H = GetSize().GetHeight();
        double r = H / 2.0;

        gc->SetBrush(wxBrush(on_ ? ThemeManager::instance().primary()
                                 : wxColour(0xD1, 0xD5, 0xDB)));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawRoundedRectangle(0, 0, W, H, r);

        int knobD = H - 4;
        int knobX = on_ ? (W - knobD - 2) : 2;
        gc->SetBrush(wxBrush(*wxWHITE));
        gc->DrawEllipse(knobX, 2, knobD, knobD);
    }

    bool on_ = true;
    std::function<void(bool)> onChange_;
};

wxBitmap loadBlackIcon(const wxString& path, int size) {
    wxImage img;
    if (!img.LoadFile(path, wxBITMAP_TYPE_PNG)) {
        return wxBitmap(size, size);
    }

    img.Rescale(size, size, wxIMAGE_QUALITY_HIGH);
    if (!img.HasAlpha()) img.InitAlpha();

    unsigned char* rgb   = img.GetData();
    unsigned char* alpha = img.GetAlpha();
    int pixels = img.GetWidth() * img.GetHeight();

    for (int i = 0; i < pixels; ++i) {
        if (alpha[i] > 0) {
            rgb[i * 3 + 0] = 0;
            rgb[i * 3 + 1] = 0;
            rgb[i * 3 + 2] = 0;
        }
    }
    return wxBitmap(img);
}

void avatarColors(Role r, wxColour& bg, wxColour& fg) {
    switch (r) {
        case Role::Admin:     bg = wxColour(0xFE, 0xE2, 0xE2); fg = wxColour(0x99, 0x1B, 0x1B); break;
        case Role::Master:    bg = wxColour(0xDB, 0xEA, 0xFE); fg = wxColour(0x1E, 0x40, 0xAF); break;
        case Role::Warehouse: bg = wxColour(0xD1, 0xFA, 0xE5); fg = wxColour(0x06, 0x5F, 0x46); break;
        default:              bg = wxColour(0xF3, 0xF4, 0xF6); fg = wxColour(0x37, 0x41, 0x51); break;
    }
}

wxString roleDisplay(Role r) {
    switch (r) {
        case Role::Admin:     return utf8::U("Администратор");
        case Role::Master:    return utf8::U("Мастер");
        case Role::Warehouse: return utf8::U("Кладовщик");
        default:              return utf8::U("Оператор");
    }
}

}

AdminWidget::AdminWidget(wxWindow* parent, const User& currentUser)
    : wxPanel(parent, wxID_ANY),
      currentUser_(currentUser)
{
    auto& tm = ThemeManager::instance();
    SetBackgroundColour(tm.background());

    auto* root = new wxBoxSizer(wxVERTICAL);
    root->AddSpacer(20);

    tabBar_ = buildTabBar();
    root->Add(tabBar_, 0, wxEXPAND | wxLEFT | wxRIGHT, 24);

    root->AddSpacer(16);

    pages_ = new wxSimplebook(this, wxID_ANY);
    pages_->SetBackgroundColour(tm.background());

    pages_->AddPage(buildUsersPage(pages_),  utf8::U("Пользователи"));
    pages_->AddPage(buildRolesPage(pages_),  utf8::U("Роли и права"));
    pages_->AddPage(buildSettingsPage(pages_), utf8::U("Настройки"));
    pages_->AddPage(buildBackupPage(pages_), utf8::U("Резервное копирование"));
    pages_->SetSelection(0);

    root->Add(pages_, 1, wxEXPAND | wxLEFT | wxRIGHT, 24);
    root->AddSpacer(20);

    SetSizer(root);

    loadUsers();
    loadBackups();
}

AdminWidget::~AdminWidget() = default;

wxPanel* AdminWidget::buildTabBar() {
    auto& tm = ThemeManager::instance();

    auto* bar = new wxPanel(this, wxID_ANY);
    bar->SetBackgroundColour(tm.background());

    auto* sizer = new wxBoxSizer(wxHORIZONTAL);

    const wxString labels[] = {
        utf8::U("Пользователи"),
        utf8::U("Роли и права"),
        utf8::U("Настройки"),
        utf8::U("Резервное копирование")
    };

    for (int i = 0; i < 4; ++i) {
        auto* item = buildTabItem(bar, labels[i], i);
        tabItems_.push_back(item);
        sizer->Add(item, 0, wxALIGN_CENTER_VERTICAL);
        if (i < 3) sizer->AddSpacer(28);
    }

    bar->SetSizer(sizer);
    return bar;
}

wxPanel* AdminWidget::buildTabItem(wxWindow* parent, const wxString& label, int index) {
    auto& tm = ThemeManager::instance();

    auto* item = new wxPanel(parent, wxID_ANY);
    item->SetBackgroundColour(tm.background());
    item->SetCursor(wxCursor(wxCURSOR_HAND));

    auto* sizer = new wxBoxSizer(wxVERTICAL);

    auto* text = new wxStaticText(item, wxID_ANY, label);
    text->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                         index == 0 ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL));
    text->SetForegroundColour(index == 0 ? tm.primary() : tm.muted());
    text->SetBackgroundColour(tm.background());

    auto* underline = new wxPanel(item, wxID_ANY, wxDefaultPosition, wxSize(-1, 2));
    underline->SetBackgroundColour(index == 0 ? tm.primary() : tm.background());

    sizer->Add(text, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP | wxBOTTOM, 6);
    sizer->Add(underline, 0, wxEXPAND);

    item->SetSizer(sizer);

    tabLabels_.push_back(text);
    tabUnderlines_.push_back(underline);

    item->Bind(wxEVT_LEFT_UP, [this, index](wxMouseEvent&) { switchTab(index); });
    text->Bind(wxEVT_LEFT_UP, [this, index](wxMouseEvent&) { switchTab(index); });

    return item;
}

void AdminWidget::switchTab(int index) {
    if (index == activeTab_) return;
    activeTab_ = index;

    auto& tm = ThemeManager::instance();

    for (size_t i = 0; i < tabLabels_.size(); ++i) {
        bool active = (static_cast<int>(i) == index);
        tabLabels_[i]->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                                      active ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL));
        tabLabels_[i]->SetForegroundColour(active ? tm.primary() : tm.muted());
        tabUnderlines_[i]->SetBackgroundColour(active ? tm.primary() : tm.background());
        tabLabels_[i]->GetParent()->Layout();
    }

    pages_->SetSelection(index);
    tabBar_->Layout();

    if (index == 3) {
        loadBackups();
    }
}

wxPanel* AdminWidget::buildUsersPage(wxWindow* parent) {
    auto& tm = ThemeManager::instance();

    auto* card = new wxPanel(parent, wxID_ANY);
    card->SetBackgroundColour(tm.surface());
    usersCard_ = card;

    auto* root = new wxBoxSizer(wxVERTICAL);

    auto* headerPanel = new wxPanel(card, wxID_ANY, wxDefaultPosition, wxSize(-1, 64));
    headerPanel->SetBackgroundColour(tm.surface());

    auto* headerSizer = new wxBoxSizer(wxHORIZONTAL);

    auto* title = new wxStaticText(headerPanel, wxID_ANY, utf8::U("Пользователи системы"));
    title->SetFont(wxFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    title->SetForegroundColour(tm.text());
    title->SetBackgroundColour(tm.surface());

    auto* addBtn = new wxButton(headerPanel, wxID_ANY, utf8::U("+ Добавить"),
                                wxDefaultPosition, wxSize(140, 36), wxBORDER_NONE);
    addBtn->SetBackgroundColour(tm.primary());
    addBtn->SetForegroundColour(*wxWHITE);
    addBtn->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    addBtn->Bind(wxEVT_BUTTON, &AdminWidget::onAddUser, this);

    headerSizer->Add(title, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 24);
    headerSizer->AddStretchSpacer(1);
    headerSizer->Add(addBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 24);

    headerPanel->SetSizer(headerSizer);
    root->Add(headerPanel, 0, wxEXPAND);

    auto* sep = new wxPanel(card, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
    sep->SetBackgroundColour(tm.border());
    root->Add(sep, 0, wxEXPAND);

    root->Add(createColumnHeader(card), 0, wxEXPAND);

    auto* sep2 = new wxPanel(card, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
    sep2->SetBackgroundColour(tm.border());
    root->Add(sep2, 0, wxEXPAND);

    usersList_ = new wxScrolledWindow(card, wxID_ANY, wxDefaultPosition,
                                      wxSize(-1, 400), wxVSCROLL | wxBORDER_NONE);
    usersList_->SetBackgroundColour(tm.surface());
    usersList_->SetScrollRate(0, 20);

    root->Add(usersList_, 1, wxEXPAND);

    card->SetSizer(root);
    return card;
}

wxPanel* AdminWidget::createColumnHeader(wxWindow* parent) {
    auto& tm = ThemeManager::instance();

    auto* header = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 40));
    header->SetBackgroundColour(wxColour(0xF9, 0xFA, 0xFB));

    auto* sizer = new wxBoxSizer(wxHORIZONTAL);

    auto addCol = [&](const wxString& text, int width, int leftPad) {
        auto* lbl = new wxStaticText(header, wxID_ANY, text);
        lbl->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        lbl->SetForegroundColour(tm.muted());
        lbl->SetBackgroundColour(wxColour(0xF9, 0xFA, 0xFB));
        sizer->Add(lbl, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, leftPad);
        lbl->SetMinSize(wxSize(width - leftPad, -1));
    };

    addCol(utf8::U("ЛОГИН"),          260, 24);
    addCol(utf8::U("РОЛЬ"),           180, 12);
    addCol(utf8::U("ПОСЛЕДНИЙ ВХОД"), 220, 12);
    addCol(utf8::U("СТАТУС"),         140, 12);
    addCol(utf8::U("ДЕЙСТВИЯ"),       120, 12);

    header->SetSizer(sizer);
    return header;
}

void AdminWidget::loadUsers() {
    allUsers_.clear();

    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return;

    const char* sql = "SELECT id, login, password_hash, role, is_active "
                      "FROM users ORDER BY id;";
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
        if (roleStr == "admin")           u.role = Role::Admin;
        else if (roleStr == "master")     u.role = Role::Master;
        else if (roleStr == "warehouse")  u.role = Role::Warehouse;
        else                              u.role = Role::Operator;
        u.isActive = sqlite3_column_int(stmt, 4) == 1;
        allUsers_.push_back(u);
    }
    sqlite3_finalize(stmt);

    refreshUsersList();
}

void AdminWidget::refreshUsersList() {
    if (!usersList_) return;

    usersList_->DestroyChildren();

    auto* sizer = new wxBoxSizer(wxVERTICAL);

    for (const auto& u : allUsers_) {
        sizer->Add(createUserRow(usersList_, u), 0, wxEXPAND);

        auto* sep = new wxPanel(usersList_, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
        sep->SetBackgroundColour(ThemeManager::instance().border());
        sizer->Add(sep, 0, wxEXPAND);
    }

    usersList_->SetSizer(sizer);
    usersList_->FitInside();
    usersList_->Layout();
    usersCard_->Layout();
}

wxPanel* AdminWidget::createUserRow(wxWindow* parent, const User& u) {
    auto& tm = ThemeManager::instance();

    auto* row = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 60));
    row->SetBackgroundColour(tm.surface());

    auto* sizer = new wxBoxSizer(wxHORIZONTAL);

    auto* loginCell = new wxPanel(row, wxID_ANY, wxDefaultPosition, wxSize(260, -1));
    loginCell->SetBackgroundColour(tm.surface());

    auto* loginSizer = new wxBoxSizer(wxHORIZONTAL);

    wxString letter = wxString::FromUTF8(u.login).Left(1).Upper();
    wxColour avBg, avFg;
    avatarColors(u.role, avBg, avFg);

    auto* avatar = new UserAvatar(loginCell, letter, avBg, avFg, 32);
    loginSizer->Add(avatar, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 16);
    loginSizer->AddSpacer(12);

    auto* loginLabel = new wxStaticText(loginCell, wxID_ANY, wxString::FromUTF8(u.login));
    loginLabel->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    loginLabel->SetForegroundColour(tm.text());
    loginLabel->SetBackgroundColour(tm.surface());
    loginSizer->Add(loginLabel, 0, wxALIGN_CENTER_VERTICAL);

    bool isSelf = false;
    if (currentUser_.id != 0 && u.id == currentUser_.id) isSelf = true;
    if (!currentUser_.login.empty() && u.login == currentUser_.login) isSelf = true;

    if (isSelf) {
        loginSizer->AddSpacer(8);
        auto* badge = new Badge(loginCell, utf8::U("Вы"),
                                wxColour(0xDB, 0xEA, 0xFE),
                                wxColour(0x1E, 0x40, 0xAF));
        loginSizer->Add(badge, 0, wxALIGN_CENTER_VERTICAL);
    }

    loginSizer->AddStretchSpacer(1);
    loginCell->SetSizer(loginSizer);
    sizer->Add(loginCell, 0, wxALIGN_CENTER_VERTICAL);

    auto* roleCell = new wxPanel(row, wxID_ANY, wxDefaultPosition, wxSize(180, -1));
    roleCell->SetBackgroundColour(tm.surface());
    auto* roleSizer = new wxBoxSizer(wxHORIZONTAL);

    auto* roleLabel = new wxStaticText(roleCell, wxID_ANY, roleDisplay(u.role));
    roleLabel->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    roleLabel->SetForegroundColour(tm.text());
    roleLabel->SetBackgroundColour(tm.surface());
    roleSizer->Add(roleLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 12);
    roleSizer->AddStretchSpacer(1);
    roleCell->SetSizer(roleSizer);
    sizer->Add(roleCell, 0, wxALIGN_CENTER_VERTICAL);

    auto* dateCell = new wxPanel(row, wxID_ANY, wxDefaultPosition, wxSize(220, -1));
    dateCell->SetBackgroundColour(tm.surface());
    auto* dateSizer = new wxBoxSizer(wxHORIZONTAL);

    auto* dateLabel = new wxStaticText(dateCell, wxID_ANY, utf8::U("—"));
    dateLabel->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    dateLabel->SetForegroundColour(tm.muted());
    dateLabel->SetBackgroundColour(tm.surface());
    dateSizer->Add(dateLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 12);
    dateSizer->AddStretchSpacer(1);
    dateCell->SetSizer(dateSizer);
    sizer->Add(dateCell, 0, wxALIGN_CENTER_VERTICAL);

    auto* statusCell = new wxPanel(row, wxID_ANY, wxDefaultPosition, wxSize(140, -1));
    statusCell->SetBackgroundColour(tm.surface());
    auto* statusSizer = new wxBoxSizer(wxHORIZONTAL);

    wxColour dotColor = u.isActive ? tm.success() : wxColour(0x9C, 0xA3, 0xAF);
    wxColour textColor = u.isActive ? tm.success() : wxColour(0x9C, 0xA3, 0xAF);

    auto* dot = new StatusDot(statusCell, dotColor, 8);
    statusSizer->Add(dot, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 12);
    statusSizer->AddSpacer(8);

    auto* statusLabel = new wxStaticText(statusCell, wxID_ANY,
                                         u.isActive ? utf8::U("Активен") : utf8::U("Отключён"));
    statusLabel->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    statusLabel->SetForegroundColour(textColor);
    statusLabel->SetBackgroundColour(tm.surface());
    statusSizer->Add(statusLabel, 0, wxALIGN_CENTER_VERTICAL);
    statusSizer->AddStretchSpacer(1);
    statusCell->SetSizer(statusSizer);
    sizer->Add(statusCell, 0, wxALIGN_CENTER_VERTICAL);

    auto* actionsCell = new wxPanel(row, wxID_ANY, wxDefaultPosition, wxSize(220, -1));
    actionsCell->SetBackgroundColour(tm.surface());
    auto* actSizer = new wxBoxSizer(wxHORIZONTAL);

    int uid = u.id;
    bool active = u.isActive;

    auto makeIconButton = [&](const wxString& path,
                              const wxString& tip,
                              std::function<void()> onClick) -> wxStaticBitmap*
    {
        wxBitmap bmp = loadBlackIcon(path, 18);
        auto* btn = new wxStaticBitmap(actionsCell, wxID_ANY, bmp);
        btn->SetBackgroundColour(tm.surface());
        btn->SetCursor(wxCursor(wxCURSOR_HAND));
        btn->SetToolTip(tip);
        btn->Bind(wxEVT_LEFT_UP, [onClick](wxMouseEvent&) { onClick(); });
        return btn;
    };

    auto* editBtn = makeIconButton(ICON_EDIT_ROLE,
        utf8::U("Изменить роль или пароль"),
        [this, uid, actionsCell]() {
            wxMenu menu;
            menu.Append(1, utf8::U("Изменить роль"));
            menu.Append(2, utf8::U("Сменить пароль"));
            int choice = actionsCell->GetPopupMenuSelectionFromUser(menu);
            if (choice == 1) onEditUser(uid);
            else if (choice == 2) onResetPassword(uid);
        });

    auto* keyBtn = makeIconButton(ICON_KEY_PASS,
        active ? utf8::U("Отключить пользователя") : utf8::U("Включить пользователя"),
        [this, uid]() { onToggleActive(uid); });

    actSizer->Add(editBtn, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 16);
    actSizer->AddSpacer(22);
    actSizer->Add(keyBtn, 0, wxALIGN_CENTER_VERTICAL);

    if (!isSelf) {
        auto* delBtn = makeIconButton(ICON_DELETE, utf8::U("Удалить"),
            [this, uid]() { onDeleteUser(uid); });
        actSizer->AddSpacer(22);
        actSizer->Add(delBtn, 0, wxALIGN_CENTER_VERTICAL);
    }

    actSizer->AddStretchSpacer(1);
    actionsCell->SetSizer(actSizer);
    sizer->Add(actionsCell, 0, wxALIGN_CENTER_VERTICAL);

    row->SetSizer(sizer);
    return row;
}

void AdminWidget::onAddUser(wxCommandEvent&) {
    wxString login = wxGetTextFromUser(utf8::U("Логин:"),
                                       utf8::U("Добавить пользователя"), "", this);
    if (login.IsEmpty()) return;

    wxString password = wxGetTextFromUser(utf8::U("Пароль:"),
                                          utf8::U("Добавить пользователя"), "", this);
    if (password.IsEmpty()) return;

    std::string loginUtf8 = std::string(login.ToUTF8().data());
    std::string passUtf8  = std::string(password.ToUTF8().data());
    std::string hash      = AuthService::hashPassword(passUtf8);

    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return;

    const char* sql = "INSERT INTO users(login, password_hash, role, is_active) "
                      "VALUES(?, ?, 'operator', 1);";
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

    Logger::instance().log(currentUser_.id, "Добавлен пользователь: " + loginUtf8, "user", 0);
    loadUsers();
}

void AdminWidget::onEditUser(int userId) {
    wxArrayString choices;
    choices.Add(utf8::U("Оператор"));
    choices.Add(utf8::U("Мастер"));
    choices.Add(utf8::U("Кладовщик"));
    choices.Add(utf8::U("Администратор"));

    wxString selected = wxGetSingleChoice(utf8::U("Выберите новую роль:"),
                                          utf8::U("Смена роли"), choices, this);
    if (selected.IsEmpty()) return;

    std::string roleStr;
    if (selected == utf8::U("Администратор")) roleStr = "admin";
    else if (selected == utf8::U("Мастер"))   roleStr = "master";
    else if (selected == utf8::U("Кладовщик"))roleStr = "warehouse";
    else                                       roleStr = "operator";

    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return;

    const char* sql = "UPDATE users SET role = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return;
    sqlite3_bind_text(stmt, 1, roleStr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, userId);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    Logger::instance().log(currentUser_.id,
        "Изменена роль id=" + std::to_string(userId) + " на " + roleStr, "user", userId);
    loadUsers();
}

void AdminWidget::onResetPassword(int userId) {
    wxString password = wxGetTextFromUser(utf8::U("Новый пароль:"),
                                          utf8::U("Смена пароля"), "", this);
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

    Logger::instance().log(currentUser_.id,
        "Сброшен пароль id=" + std::to_string(userId), "user", userId);
    wxMessageBox(utf8::U("Пароль изменён"), utf8::U("Готово"),
                 wxOK | wxICON_INFORMATION, this);
}

void AdminWidget::onToggleActive(int userId) {
    if (userId == currentUser_.id) {
        wxMessageBox(utf8::U("Нельзя отключить себя"),
                     utf8::U("Ошибка"), wxOK | wxICON_WARNING, this);
        return;
    }

    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return;

    const char* sql = "UPDATE users SET is_active = "
                      "CASE is_active WHEN 1 THEN 0 ELSE 1 END WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return;
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    Logger::instance().log(currentUser_.id,
        "Изменён статус id=" + std::to_string(userId), "user", userId);
    loadUsers();
}

void AdminWidget::onDeleteUser(int userId) {
    if (userId == currentUser_.id) {
        wxMessageBox(utf8::U("Нельзя удалить себя"),
                     utf8::U("Ошибка"), wxOK | wxICON_WARNING, this);
        return;
    }

    int answer = wxMessageBox(utf8::U("Удалить пользователя?"),
                              utf8::U("Подтверждение"),
                              wxYES_NO | wxICON_QUESTION, this);
    if (answer != wxYES) return;

    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return;

    const char* sql = "DELETE FROM users WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return;
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    Logger::instance().log(currentUser_.id,
        "Удалён пользователь id=" + std::to_string(userId), "user", userId);
    loadUsers();
}

wxPanel* AdminWidget::buildRolesPage(wxWindow* parent) {
    auto& tm = ThemeManager::instance();

    auto* page = new wxPanel(parent, wxID_ANY);
    page->SetBackgroundColour(tm.background());

    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->AddSpacer(16);

    struct RoleInfo { const char* name; const char* desc; const char* perms; wxColour color; };
    RoleInfo roles[] = {
        { "Администратор", "Полный доступ ко всем разделам системы",
          "Заказы (чтение/запись)  |  Склад (чтение/запись)  |  Отчёты  |  Администрирование",
          wxColour(0xEF, 0x44, 0x44) },
        { "Мастер", "Работа с заказами, просмотр склада",
          "Заказы (чтение/запись)  |  Склад (только чтение)  |  Отчёты (только чтение)",
          wxColour(0xF5, 0x9E, 0x0B) },
        { "Кладовщик", "Управление складом, просмотр заказов",
          "Заказы (только чтение)  |  Склад (чтение/запись)",
          wxColour(0x10, 0xB9, 0x81) },
        { "Оператор", "Приём заказов, работа с клиентами",
          "Заказы (чтение/запись)",
          wxColour(0x25, 0x63, 0xEB) }
    };

    for (auto& r : roles) {
        auto* card = new wxPanel(page, wxID_ANY);
        card->SetBackgroundColour(tm.surface());

        auto* cardSizer = new wxBoxSizer(wxHORIZONTAL);

        auto* strip = new wxPanel(card, wxID_ANY, wxDefaultPosition, wxSize(6, -1));
        strip->SetBackgroundColour(r.color);
        cardSizer->Add(strip, 0, wxEXPAND);

        auto* content = new wxBoxSizer(wxVERTICAL);

        auto* name = new wxStaticText(card, wxID_ANY, utf8::U(r.name));
        name->SetFont(wxFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        name->SetForegroundColour(tm.text());
        name->SetBackgroundColour(tm.surface());

        auto* desc = new wxStaticText(card, wxID_ANY, utf8::U(r.desc));
        desc->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        desc->SetForegroundColour(tm.muted());
        desc->SetBackgroundColour(tm.surface());

        auto* perms = new wxStaticText(card, wxID_ANY, utf8::U(r.perms));
        perms->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        perms->SetForegroundColour(tm.muted());
        perms->SetBackgroundColour(tm.surface());

        content->AddSpacer(16);
        content->Add(name, 0, wxLEFT, 20);
        content->AddSpacer(4);
        content->Add(desc, 0, wxLEFT, 20);
        content->AddSpacer(8);
        content->Add(perms, 0, wxLEFT | wxBOTTOM, 20);

        cardSizer->Add(content, 1, wxEXPAND);
        card->SetSizer(cardSizer);
        sizer->Add(card, 0, wxEXPAND | wxBOTTOM, 12);
    }

    page->SetSizer(sizer);
    return page;
}

wxPanel* AdminWidget::buildSettingsPage(wxWindow* parent) {
    auto& tm = ThemeManager::instance();

    auto* page = new wxPanel(parent, wxID_ANY);
    page->SetBackgroundColour(tm.background());

    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->AddSpacer(16);

    auto* card = new wxPanel(page, wxID_ANY);
    card->SetBackgroundColour(tm.surface());

    auto* cardSizer = new wxBoxSizer(wxVERTICAL);

    struct Field { const char* label; const char* value; };
    Field fields[] = {
        { "Название сервисного центра", "ООО «Сервис Плюс»" },
        { "Телефон",                    "+7 (495) 000-00-00" },
        { "Адрес",                      "г. Москва, ул. Примерная, 1" },
        { "Email",                      "info@service.ru" }
    };

    for (auto& f : fields) {
        auto* label = new wxStaticText(card, wxID_ANY, utf8::U(f.label));
        label->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        label->SetForegroundColour(tm.muted());
        label->SetBackgroundColour(tm.surface());
        cardSizer->Add(label, 0, wxLEFT | wxRIGHT | wxTOP, 20);
        cardSizer->AddSpacer(6);

        auto* field = new wxTextCtrl(card, wxID_ANY, utf8::U(f.value),
                                     wxDefaultPosition, wxSize(-1, 36), wxBORDER_SIMPLE);
        field->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        field->SetBackgroundColour(tm.surface());
        field->SetForegroundColour(tm.text());
        cardSizer->Add(field, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    }

    auto* saveBtn = new wxButton(card, wxID_ANY, utf8::U("Сохранить настройки"),
                                 wxDefaultPosition, wxSize(220, 40), wxBORDER_NONE);
    saveBtn->SetBackgroundColour(tm.primary());
    saveBtn->SetForegroundColour(*wxWHITE);
    saveBtn->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    cardSizer->Add(saveBtn, 0, wxALIGN_RIGHT | wxALL, 20);

    card->SetSizer(cardSizer);
    sizer->Add(card, 0, wxEXPAND);

    page->SetSizer(sizer);
    return page;
}

wxPanel* AdminWidget::buildBackupPage(wxWindow* parent) {
    auto& tm = ThemeManager::instance();

    auto* page = new wxPanel(parent, wxID_ANY);
    page->SetBackgroundColour(tm.background());

    auto* root = new wxBoxSizer(wxVERTICAL);
    root->AddSpacer(16);

    auto* topRow = new wxBoxSizer(wxHORIZONTAL);

    auto* card1 = new wxPanel(page, wxID_ANY, wxDefaultPosition, wxSize(-1, 200));
    card1->SetBackgroundColour(tm.surface());

    auto* c1 = new wxBoxSizer(wxVERTICAL);

    auto* c1Title = new wxStaticText(card1, wxID_ANY, utf8::U("Создать резервную копию"));
    c1Title->SetFont(wxFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    c1Title->SetForegroundColour(tm.text());
    c1Title->SetBackgroundColour(tm.surface());

    auto* c1Desc = new wxStaticText(card1, wxID_ANY,
        utf8::U("Сохраните текущее состояние базы данных. Копия\nбудет создана в директории резервных копий."));
    c1Desc->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    c1Desc->SetForegroundColour(tm.muted());
    c1Desc->SetBackgroundColour(tm.surface());

    auto* backupBtn = new wxButton(card1, wxID_ANY, utf8::U("Создать копию"),
                                   wxDefaultPosition, wxSize(200, 40), wxBORDER_NONE);
    backupBtn->SetBackgroundColour(tm.primary());
    backupBtn->SetForegroundColour(*wxWHITE);
    backupBtn->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    backupBtn->Bind(wxEVT_BUTTON, &AdminWidget::onBackup, this);

    c1->Add(c1Title, 0, wxLEFT | wxRIGHT | wxTOP, 24);
    c1->AddSpacer(12);
    c1->Add(c1Desc, 0, wxLEFT | wxRIGHT, 24);
    c1->AddSpacer(20);
    c1->Add(backupBtn, 0, wxLEFT | wxRIGHT | wxBOTTOM, 24);
    card1->SetSizer(c1);

    auto* card2 = new wxPanel(page, wxID_ANY, wxDefaultPosition, wxSize(-1, 200));
    card2->SetBackgroundColour(tm.surface());

    auto* c2 = new wxBoxSizer(wxVERTICAL);

    auto* c2HeaderRow = new wxBoxSizer(wxHORIZONTAL);
    auto* c2Title = new wxStaticText(card2, wxID_ANY, utf8::U("Автоматическое резервирование"));
    c2Title->SetFont(wxFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    c2Title->SetForegroundColour(tm.text());
    c2Title->SetBackgroundColour(tm.surface());

    auto& cfg = ConfigManager::instance();
    auto* toggle = new ToggleSwitch(card2, cfg.autoBackupEnabled());
    toggle->setOnChange([](bool on) {
        ConfigManager::instance().setAutoBackupEnabled(on);
    });

    c2HeaderRow->Add(c2Title, 1, wxALIGN_CENTER_VERTICAL);
    c2HeaderRow->Add(toggle, 0, wxALIGN_CENTER_VERTICAL);

    auto* c2Desc = new wxStaticText(card2, wxID_ANY, utf8::U("Создавать копии ежедневно"));
    c2Desc->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    c2Desc->SetForegroundColour(tm.muted());
    c2Desc->SetBackgroundColour(tm.surface());

    auto* c2Time = new wxStaticText(card2, wxID_ANY, utf8::U("Время создания: 08:00"));
    c2Time->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    c2Time->SetForegroundColour(tm.muted());
    c2Time->SetBackgroundColour(tm.surface());

    auto* c2Keep = new wxStaticText(card2, wxID_ANY, utf8::U("Хранить последние: 30 копий"));
    c2Keep->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    c2Keep->SetForegroundColour(tm.muted());
    c2Keep->SetBackgroundColour(tm.surface());

    c2->Add(c2HeaderRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 24);
    c2->AddSpacer(12);
    c2->Add(c2Desc, 0, wxLEFT | wxRIGHT, 24);
    c2->AddSpacer(14);
    c2->Add(c2Time, 0, wxLEFT | wxRIGHT, 24);
    c2->AddSpacer(4);
    c2->Add(c2Keep, 0, wxLEFT | wxRIGHT | wxBOTTOM, 24);
    card2->SetSizer(c2);

    topRow->Add(card1, 1, wxEXPAND | wxRIGHT, 16);
    topRow->Add(card2, 1, wxEXPAND);
    root->Add(topRow, 0, wxEXPAND);

    root->AddSpacer(20);

    auto* listCard = new wxPanel(page, wxID_ANY);
    listCard->SetBackgroundColour(tm.surface());
    backupsCard_ = listCard;

    auto* listRoot = new wxBoxSizer(wxVERTICAL);

    auto* headerPanel = new wxPanel(listCard, wxID_ANY, wxDefaultPosition, wxSize(-1, 44));
    headerPanel->SetBackgroundColour(wxColour(0xF9, 0xFA, 0xFB));

    auto* headerSizer = new wxBoxSizer(wxHORIZONTAL);

    auto addHdr = [&](const wxString& text, int width) {
        auto* lbl = new wxStaticText(headerPanel, wxID_ANY, text);
        lbl->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        lbl->SetForegroundColour(tm.muted());
        lbl->SetBackgroundColour(wxColour(0xF9, 0xFA, 0xFB));
        lbl->SetMinSize(wxSize(width, -1));
        headerSizer->Add(lbl, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 16);
    };

    addHdr(utf8::U("ИМЯ ФАЙЛА"),     400);
    addHdr(utf8::U("РАЗМЕР"),        140);
    addHdr(utf8::U("ДАТА СОЗДАНИЯ"), 240);
    addHdr(utf8::U("ТИП"),           130);
    addHdr(utf8::U("ДЕЙСТВИЯ"),      180);

    headerPanel->SetSizer(headerSizer);
    listRoot->Add(headerPanel, 0, wxEXPAND);

    auto* sep = new wxPanel(listCard, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
    sep->SetBackgroundColour(tm.border());
    listRoot->Add(sep, 0, wxEXPAND);

    backupsList_ = new wxScrolledWindow(listCard, wxID_ANY, wxDefaultPosition,
                                        wxSize(-1, 260), wxVSCROLL | wxBORDER_NONE);
    backupsList_->SetBackgroundColour(tm.surface());
    backupsList_->SetScrollRate(0, 20);
    listRoot->Add(backupsList_, 1, wxEXPAND);

    listCard->SetSizer(listRoot);
    root->Add(listCard, 1, wxEXPAND);

    page->SetSizer(root);
    return page;
}

void AdminWidget::loadBackups() {
    backups_.clear();

    auto& cfg = ConfigManager::instance();
    fs::path dbPath = cfg.dbPath();
    fs::path backupDir = dbPath.parent_path() / ".." / "backups";

    if (!fs::exists(backupDir)) {
        rebuildBackupRows();
        return;
    }

    for (auto& entry : fs::directory_iterator(backupDir)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".db") continue;

        BackupEntry be;
        be.fileName = entry.path().filename().string();

        auto sz = fs::file_size(entry.path());
        be.sizeMb = sz / 1024.0 / 1024.0;

        try {
            auto ftime = fs::last_write_time(entry.path());
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
            std::time_t t = std::chrono::system_clock::to_time_t(sctp);
            std::tm* tm2 = std::localtime(&t);
            char buf[64];
            std::strftime(buf, sizeof(buf), "%d.%m.%Y %H:%M", tm2);
            be.dateStr = buf;
        } catch (...) {
            be.dateStr = "—";
        }

        be.isAuto = (be.fileName.rfind("auto_", 0) == 0);

        backups_.push_back(be);
    }

    std::sort(backups_.begin(), backups_.end(),
              [](const BackupEntry& a, const BackupEntry& b) {
                  return a.dateStr > b.dateStr;
              });

    rebuildBackupRows();
}

void AdminWidget::rebuildBackupRows() {
    if (!backupsList_) return;

    backupsList_->DestroyChildren();

    auto& tm = ThemeManager::instance();

    auto* sizer = new wxBoxSizer(wxVERTICAL);

    if (backups_.empty()) {
        auto* empty = new wxStaticText(backupsList_, wxID_ANY,
                                       utf8::U("Резервные копии не найдены"));
        empty->SetForegroundColour(tm.muted());
        empty->SetBackgroundColour(tm.surface());
        empty->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        sizer->Add(empty, 0, wxALL, 24);
    } else {
        for (auto& be : backups_) {
            auto* row = new wxPanel(backupsList_, wxID_ANY,
                                    wxDefaultPosition, wxSize(-1, 44));
            row->SetBackgroundColour(tm.surface());

            auto* rowSizer = new wxBoxSizer(wxHORIZONTAL);

            auto addCellText = [&](const wxString& text, int width, wxColour fg) {
                auto* lbl = new wxStaticText(row, wxID_ANY, text);
                lbl->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                                    wxFONTWEIGHT_NORMAL));
                lbl->SetForegroundColour(fg);
                lbl->SetBackgroundColour(tm.surface());
                lbl->SetMinSize(wxSize(width, -1));
                rowSizer->Add(lbl, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 16);
            };

            addCellText(utf8::U(be.fileName.c_str()), 400, tm.text());

            wxString sizeStr = wxString::Format("%.1f ", be.sizeMb) + utf8::U("МБ");
            addCellText(sizeStr, 140, tm.text());

            addCellText(utf8::U(be.dateStr.c_str()), 240, tm.muted());

            auto* typeCell = new wxPanel(row, wxID_ANY, wxDefaultPosition, wxSize(130, -1));
            typeCell->SetBackgroundColour(tm.surface());
            auto* typeSizer = new wxBoxSizer(wxHORIZONTAL);

            wxColour badgeBg = be.isAuto ? wxColour(0xD1, 0xFA, 0xE5)
                                          : wxColour(0xE5, 0xE7, 0xEB);
            wxColour badgeFg = be.isAuto ? wxColour(0x06, 0x5F, 0x46)
                                          : wxColour(0x37, 0x41, 0x51);
            wxString badgeText = be.isAuto ? utf8::U("Авто") : utf8::U("Ручн.");

            auto* badge = new Badge(typeCell, badgeText, badgeBg, badgeFg);
            typeSizer->Add(badge, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 12);
            typeSizer->AddStretchSpacer(1);
            typeCell->SetSizer(typeSizer);
            rowSizer->Add(typeCell, 0, wxALIGN_CENTER_VERTICAL);

            auto* actCell = new wxPanel(row, wxID_ANY, wxDefaultPosition, wxSize(180, -1));
            actCell->SetBackgroundColour(tm.surface());
            auto* actSizer = new wxBoxSizer(wxHORIZONTAL);

            std::string fn = be.fileName;

            auto makeIcon = [&](const wxBitmap& bmp, const wxString& tip,
                                std::function<void()> onClick) -> wxStaticBitmap*
            {
                auto* btn = new wxStaticBitmap(actCell, wxID_ANY, bmp);
                btn->SetBackgroundColour(tm.surface());
                btn->SetCursor(wxCursor(wxCURSOR_HAND));
                btn->SetToolTip(tip);
                btn->Bind(wxEVT_LEFT_UP, [onClick](wxMouseEvent&) { onClick(); });
                return btn;
            };

            wxBitmap restoreBmp = wxArtProvider::GetBitmap(
                wxART_REDO, wxART_BUTTON, wxSize(18, 18));
            if (!restoreBmp.IsOk())
                restoreBmp = wxArtProvider::GetBitmap(
                    wxART_GO_BACK, wxART_BUTTON, wxSize(18, 18));

            auto* restoreBtn = makeIcon(
                restoreBmp,
                utf8::U("Восстановить из копии"),
                [this, fn]() {
                    auto& cfg = ConfigManager::instance();
                    fs::path dbPath = cfg.dbPath();
                    fs::path backupFile = dbPath.parent_path() / ".." / "backups" / fn;

                    int answer = wxMessageBox(
                        utf8::U("Восстановить базу данных из этой копии?\n"
                                "Текущие данные будут перезаписаны."),
                        utf8::U("Восстановление"),
                        wxYES_NO | wxICON_WARNING, this);
                    if (answer != wxYES) return;

                    try {
                        fs::copy_file(backupFile, dbPath,
                                      fs::copy_options::overwrite_existing);
                        Logger::instance().log(currentUser_.id,
                            "Восстановлена БД из копии: " + fn, "db", 0);
                        wxMessageBox(utf8::U("База данных восстановлена.\n"
                                             "Перезапустите приложение."),
                                     utf8::U("Готово"), wxOK | wxICON_INFORMATION, this);
                    } catch (...) {
                        wxMessageBox(utf8::U("Не удалось восстановить базу данных"),
                                     utf8::U("Ошибка"), wxOK | wxICON_ERROR, this);
                    }
                });

            wxBitmap saveBmp = wxArtProvider::GetBitmap(
                wxART_GO_DOWN, wxART_BUTTON, wxSize(18, 18));
            if (!saveBmp.IsOk())
                saveBmp = wxArtProvider::GetBitmap(
                    wxART_FILE_SAVE_AS, wxART_BUTTON, wxSize(18, 18));

            auto* saveBtn = makeIcon(
                saveBmp,
                utf8::U("Сохранить копию"),
                [this, fn]() {
                    auto& cfg = ConfigManager::instance();
                    fs::path dbPath = cfg.dbPath();
                    fs::path src = dbPath.parent_path() / ".." / "backups" / fn;

                    wxString dst = wxFileSelector(utf8::U("Сохранить копию как"),
                                                  "", fn.c_str(), "*.db",
                                                  "DB files (*.db)|*.db",
                                                  wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
                    if (dst.IsEmpty()) return;
                    try {
                        fs::copy_file(src, fs::path(dst.ToStdString()),
                                      fs::copy_options::overwrite_existing);
                    } catch (...) {
                        wxMessageBox(utf8::U("Не удалось сохранить копию"),
                                     utf8::U("Ошибка"), wxOK | wxICON_ERROR, this);
                    }
                });

            wxBitmap delBmp = loadBlackIcon("resources/icons/trash.png", 18);

            auto* delBtn = makeIcon(
                delBmp,
                utf8::U("Удалить"),
                [this, fn]() {
                    int answer = wxMessageBox(
                        utf8::U("Удалить выбранную резервную копию?"),
                        utf8::U("Подтверждение"),
                        wxYES_NO | wxICON_QUESTION, this);
                    if (answer != wxYES) return;

                    auto& cfg = ConfigManager::instance();
                    fs::path dbPath = cfg.dbPath();
                    fs::path file = dbPath.parent_path() / ".." / "backups" / fn;
                    try {
                        fs::remove(file);
                        Logger::instance().log(currentUser_.id,
                            "Удалена резервная копия: " + fn, "db", 0);
                        loadBackups();
                    } catch (...) {
                        wxMessageBox(utf8::U("Не удалось удалить файл"),
                                     utf8::U("Ошибка"), wxOK | wxICON_ERROR, this);
                    }
                });

            actSizer->Add(restoreBtn, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 12);
            actSizer->AddSpacer(16);
            actSizer->Add(saveBtn, 0, wxALIGN_CENTER_VERTICAL);
            actSizer->AddSpacer(16);
            actSizer->Add(delBtn, 0, wxALIGN_CENTER_VERTICAL);
            actSizer->AddStretchSpacer(1);

            actCell->SetSizer(actSizer);
            rowSizer->Add(actCell, 0, wxALIGN_CENTER_VERTICAL);

            row->SetSizer(rowSizer);

            sizer->Add(row, 0, wxEXPAND);

            auto* sep = new wxPanel(backupsList_, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
            sep->SetBackgroundColour(tm.border());
            sizer->Add(sep, 0, wxEXPAND);
        }
    }

    backupsList_->SetSizer(sizer);
    backupsList_->FitInside();
    backupsList_->Layout();

    if (backupsCard_) backupsCard_->Layout();
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
        Logger::instance().log(currentUser_.id,
            "Создана резервная копия: " + std::string(nameBuf), "db", 0);

        wxString msg = utf8::U("Резервная копия создана:\n") +
                       utf8::U(outPath.string().c_str());
        wxMessageBox(msg, utf8::U("Резервное копирование"),
                     wxOK | wxICON_INFORMATION, this);
        loadBackups();
    } catch (...) {
        wxMessageBox(utf8::U("Не удалось создать резервную копию"),
                     utf8::U("Ошибка"), wxOK | wxICON_ERROR, this);
    }
}

}