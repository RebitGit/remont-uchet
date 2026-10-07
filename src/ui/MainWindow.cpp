#include "ui/MainWindow.h"
#include "ui/DashboardWidget.h"
#include "ui/WarehouseWidget.h"
#include "ui/RoundedPanel.h"
#include "ui/SettingsDialog.h"
#include "core/ThemeManager.h"
#include "resources/styles.h"
#include "resources/utf8.h"
#include "ui/NewOrderWidget.h"
#include "ui/AdminWidget.h"
#include "ui/ReportsWidget.h"

#include <wx/statbmp.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <ctime>
#include <iostream>

namespace remont {

namespace {

wxBitmap loadWhiteIcon(const wxString& path, int size) {
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
            rgb[i * 3 + 0] = 255;
            rgb[i * 3 + 1] = 255;
            rgb[i * 3 + 2] = 255;
        }
    }
    return wxBitmap(img);
}

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

        gc->SetFont(wxFont(13, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
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

class LogoBox : public wxPanel {
public:
    LogoBox(wxWindow* parent, int size)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(size, size)),
          size_(size)
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetBackgroundColour(parent->GetBackgroundColour());
        SetMinSize(wxSize(size, size));

        icon_ = loadWhiteIcon("resources/icons/cpu.png", 24);
        Bind(wxEVT_PAINT, &LogoBox::onPaint, this);
    }

private:
    void onPaint(wxPaintEvent&) {
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
        dc.Clear();

        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        if (!gc) return;

        gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
        gc->SetBrush(wxBrush(ThemeManager::instance().primary()));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawRoundedRectangle(0, 0, size_, size_, 10);

        if (icon_.IsOk()) {
            int ix = (size_ - icon_.GetWidth()) / 2;
            int iy = (size_ - icon_.GetHeight()) / 2;
            gc->DrawBitmap(icon_, ix, iy, icon_.GetWidth(), icon_.GetHeight());
        }
    }

    int size_;
    wxBitmap icon_;
};

}

MainWindow::MainWindow(const User& user)
    : wxFrame(nullptr, wxID_ANY, utf8::U("Ремонт-Учёт"),
              wxDefaultPosition, wxSize(1280, 800)),
      user_(user)
{
    auto& tm = ThemeManager::instance();

    std::cerr << "[MAIN] dark=" << (tm.isDark() ? "yes" : "no") << "\n";
    std::cerr << "[MAIN] background rgb="
              << (int)tm.background().Red() << ","
              << (int)tm.background().Green() << ","
              << (int)tm.background().Blue() << "\n";

    SetBackgroundColour(tm.background());

    auto* root = new wxBoxSizer(wxHORIZONTAL);
    root->Add(buildSidebar(), 0, wxEXPAND);

    auto* rightCol = new wxBoxSizer(wxVERTICAL);
    rightCol->Add(buildHeader(), 0, wxEXPAND);

       book_ = new wxSimplebook(this, wxID_ANY);
    book_->SetBackgroundColour(tm.background());

    dashboardPage_ = new DashboardWidget(book_);
    newOrderPage_ = new NewOrderWidget(book_, user_);

    dashboardPage_->setOnNewOrder([this]() { showNewOrderPage(); });

    book_->AddPage(dashboardPage_, utf8::U("Заказы"));
    book_->AddPage(new WarehouseWidget(book_), utf8::U("Склад"));
        book_->AddPage(new ReportsWidget(book_), utf8::U("Отчёты"));
    book_->AddPage(new AdminWidget(book_), utf8::U("Администрирование"));
    book_->AddPage(newOrderPage_, utf8::U("Новый заказ"));

    Bind(wxEVT_BUTTON, &MainWindow::onBackToDashboard, this, 2001);

    rightCol->Add(book_, 1, wxEXPAND);
    root->Add(rightCol, 1, wxEXPAND);

    SetSizer(root);
    Maximize(true);
    setActiveNav(1001);

    Bind(wxEVT_CLOSE_WINDOW, &MainWindow::onClose, this);
}

MainWindow::~MainWindow() = default;

RoundedPanel* MainWindow::buildNavItem(wxWindow* parent, const wxString& label,
                                       const wxString& iconPath, int id, bool active)
{
    auto& tm = ThemeManager::instance();

    auto* panel = new RoundedPanel(parent,
                                   active ? tm.primary() : tm.sidebar(),
                                   8, wxSize(-1, 48));
    panel->SetCursor(wxCursor(wxCURSOR_HAND));
    panel->SetHoverColour(tm.sidebarHover());
    panel->SetIcon(loadWhiteIcon(iconPath, 20));
    panel->SetLabel(label, *wxWHITE, tm.fontSizeNormal());
    panel->SetActive(active);

    panel->Bind(wxEVT_LEFT_UP, [this, id](wxMouseEvent&) {
        wxCommandEvent evt(wxEVT_BUTTON, id);
        onNavClick(evt);
    });

    return panel;
}

wxPanel* MainWindow::buildSidebar() {
    auto& tm = ThemeManager::instance();

    sidebar_ = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(270, -1));
    sidebar_->SetBackgroundColour(tm.sidebar());

    auto* sizer = new wxBoxSizer(wxVERTICAL);

    auto* logoRow = new wxBoxSizer(wxHORIZONTAL);

    auto* logoBox = new LogoBox(sidebar_, 42);

    auto* nameCol = new wxBoxSizer(wxVERTICAL);
    auto* name = new wxStaticText(sidebar_, wxID_ANY, utf8::U("Ремонт-Учёт"));
    name->SetForegroundColour(*wxWHITE);
    name->SetBackgroundColour(tm.sidebar());
    name->SetFont(wxFont(tm.fontSizeHeader(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    auto* version = new wxStaticText(sidebar_, wxID_ANY, "v1.0.0");
    version->SetForegroundColour(wxColour(0x94, 0xA3, 0xB8));
    version->SetBackgroundColour(tm.sidebar());
    version->SetFont(wxFont(tm.fontSizeSmall() - 2, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    nameCol->Add(name);
    nameCol->Add(version);

    logoRow->Add(logoBox, 0, wxALIGN_CENTER_VERTICAL);
    logoRow->AddSpacer(12);
    logoRow->Add(nameCol, 1, wxALIGN_CENTER_VERTICAL);

    sizer->Add(logoRow, 0, wxEXPAND | wxALL, 20);
    sizer->AddSpacer(8);

    navOrders_    = buildNavItem(sidebar_, utf8::U("Заказы"),
                                 "resources/icons/nav_orders.png", 1001, true);
    navWarehouse_ = buildNavItem(sidebar_, utf8::U("Склад"),
                                 "resources/icons/nav_warehouse.png", 1002, false);
    navReports_   = buildNavItem(sidebar_, utf8::U("Отчёты"),
                                 "resources/icons/nav_reports.png", 1003, false);
    navAdmin_     = buildNavItem(sidebar_, utf8::U("Администрирование"),
                                 "resources/icons/nav_admin.png", 1004, false);

    sizer->Add(navOrders_, 0, wxEXPAND | wxLEFT | wxRIGHT, 12);
    sizer->AddSpacer(4);
    sizer->Add(navWarehouse_, 0, wxEXPAND | wxLEFT | wxRIGHT, 12);
    sizer->AddSpacer(4);
    sizer->Add(navReports_, 0, wxEXPAND | wxLEFT | wxRIGHT, 12);
    sizer->AddSpacer(4);
    sizer->Add(navAdmin_, 0, wxEXPAND | wxLEFT | wxRIGHT, 12);

    sizer->AddStretchSpacer(1);

    auto* sep = new wxPanel(sidebar_, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
    sep->SetBackgroundColour(wxColour(0x33, 0x41, 0x55));
    sizer->Add(sep, 0, wxEXPAND | wxLEFT | wxRIGHT, 16);
    sizer->AddSpacer(12);

    auto* profileRow = new wxBoxSizer(wxHORIZONTAL);

    auto* avatar = new CircleAvatar(sidebar_, "AD",
                                    wxColour(0x47, 0x55, 0x69),
                                    *wxWHITE, 42);

    auto* profCol = new wxBoxSizer(wxVERTICAL);
    auto* profRole = new wxStaticText(sidebar_, wxID_ANY, utf8::U("Администратор"));
    profRole->SetForegroundColour(*wxWHITE);
    profRole->SetBackgroundColour(tm.sidebar());
    profRole->SetFont(wxFont(tm.fontSizeSmall(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    auto* profLogin = new wxStaticText(sidebar_, wxID_ANY,
                                       wxString::FromUTF8(user_.login));
    profLogin->SetForegroundColour(wxColour(0x94, 0xA3, 0xB8));
    profLogin->SetBackgroundColour(tm.sidebar());
    profLogin->SetFont(wxFont(tm.fontSizeSmall() - 1, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    profCol->Add(profRole);
    profCol->Add(profLogin);

    auto* gearPanel = new RoundedPanel(sidebar_, tm.sidebar(), 15, wxSize(30, 30));
    gearPanel->SetCursor(wxCursor(wxCURSOR_HAND));
    gearPanel->SetHoverColour(tm.sidebarHover());
    gearPanel->SetIcon(loadWhiteIcon("resources/icons/settings.png", 18));
    gearPanel->Bind(wxEVT_LEFT_UP, &MainWindow::onSettingsClick, this);

    profileRow->Add(avatar, 0, wxALIGN_CENTER_VERTICAL);
    profileRow->AddSpacer(10);
    profileRow->Add(profCol, 1, wxALIGN_CENTER_VERTICAL);
    profileRow->Add(gearPanel, 0, wxALIGN_CENTER_VERTICAL);

    sizer->Add(profileRow, 0, wxEXPAND | wxLEFT | wxRIGHT, 16);
    sizer->AddSpacer(8);

    auto* logoutPanel = new RoundedPanel(sidebar_, tm.sidebar(), 8, wxSize(-1, 44));
    logoutPanel->SetCursor(wxCursor(wxCURSOR_HAND));
    logoutPanel->SetHoverColour(tm.sidebarHover());
    logoutPanel->SetIcon(loadWhiteIcon("resources/icons/nav_logout.png", 18));
    logoutPanel->SetLabel(utf8::U("Выйти"), wxColour(0x94, 0xA3, 0xB8), tm.fontSizeSmall());

    logoutPanel->Bind(wxEVT_LEFT_UP, &MainWindow::onLogout, this);
    sizer->Add(logoutPanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 16);

    sidebar_->SetSizer(sizer);
    return sidebar_;
}

wxPanel* MainWindow::buildHeader() {
    auto& tm = ThemeManager::instance();

    header_ = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 72));
    header_->SetBackgroundColour(tm.surface());

    auto* sizer = new wxBoxSizer(wxHORIZONTAL);

    headerTitle_ = new wxStaticText(header_, wxID_ANY, utf8::U("Заказы"));
    headerTitle_->SetFont(wxFont(tm.fontSizeTitle(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    headerTitle_->SetForegroundColour(tm.text());

    std::time_t t = std::time(nullptr);
    std::tm* tm2 = std::localtime(&t);
    char dateBuf[32];
    std::strftime(dateBuf, sizeof(dateBuf), "%d.%m.%Y", tm2);

    auto* dateLabel = new wxStaticText(header_, wxID_ANY, wxString::FromUTF8(dateBuf));
    dateLabel->SetForegroundColour(tm.muted());
    dateLabel->SetFont(wxFont(tm.fontSizeSmall(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

    auto* statusLabel = new wxStaticText(header_, wxID_ANY, utf8::U("● Система работает"));
    statusLabel->SetForegroundColour(tm.success());
    statusLabel->SetFont(wxFont(tm.fontSizeSmall(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

    sizer->Add(headerTitle_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 24);
    sizer->AddStretchSpacer(1);
    sizer->Add(dateLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);
    sizer->Add(statusLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 24);

    header_->SetSizer(sizer);
    return header_;
}

void MainWindow::setActiveNav(int id) {
    auto& tm = ThemeManager::instance();

    wxColour active = tm.primary();
    wxColour normal = tm.sidebar();

    auto setup = [&](RoundedPanel* panel, bool isActive) {
        panel->SetActive(isActive);
        panel->SetFillColour(isActive ? active : normal);
    };

    setup(navOrders_,    id == 1001);
    setup(navWarehouse_, id == 1002);
    setup(navReports_,   id == 1003);
    setup(navAdmin_,     id == 1004);
}

void MainWindow::onNavClick(wxCommandEvent& event) {
    int id = event.GetId();
    currentNavId_ = id;
    if (id == 1001) { book_->SetSelection(0); headerTitle_->SetLabel(utf8::U("Заказы")); }
    else if (id == 1002) { book_->SetSelection(1); headerTitle_->SetLabel(utf8::U("Склад запчастей")); }
    else if (id == 1003) { book_->SetSelection(2); headerTitle_->SetLabel(utf8::U("Отчёты")); }
    else if (id == 1004) { book_->SetSelection(3); headerTitle_->SetLabel(utf8::U("Администрирование")); }
    setActiveNav(id);
}

void MainWindow::onLogout(wxEvent&) {
    logoutRequested_ = true;
    Close();
}

void MainWindow::onSettingsClick(wxMouseEvent&) {
    SettingsDialog dlg(this, user_);
    int result = dlg.ShowModal();

    if (result == wxID_RETRY) {
        themeChanged_ = true;
        Close();
    }
}

void MainWindow::showNewOrderPage() {
    book_->SetSelection(4);
    headerTitle_->SetLabel(utf8::U("Новый заказ"));
    setActiveNav(1001);
}

void MainWindow::onBackToDashboard(wxCommandEvent&) {
    book_->SetSelection(0);
    headerTitle_->SetLabel(utf8::U("Заказы"));
    dashboardPage_->reload();
    setActiveNav(1001);
}

void MainWindow::onClose(wxCloseEvent& event) {
    event.Skip();
}

}