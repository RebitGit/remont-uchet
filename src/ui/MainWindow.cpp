#include "ui/MainWindow.h"
#include "ui/DashboardWidget.h"
#include "resources/styles.h"
#include "resources/utf8.h"

#include <wx/statbmp.h>
#include <ctime>

namespace remont {

namespace {

wxBitmap loadWhiteIcon(const wxString& path, int size) {
    wxImage img;
    if (!img.LoadFile(path, wxBITMAP_TYPE_PNG)) {
        return wxBitmap(size, size);
    }
    img.Rescale(size, size, wxIMAGE_QUALITY_HIGH);

    if (!img.HasAlpha()) {
        img.InitAlpha();
    }

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

}

MainWindow::MainWindow(const User& user)
    : wxFrame(nullptr, wxID_ANY, utf8::U("Ремонт-Учёт"),
              wxDefaultPosition, wxSize(1280, 800)),
      user_(user)
{
    SetBackgroundColour(styles::Background);

    auto* root = new wxBoxSizer(wxHORIZONTAL);
    root->Add(buildSidebar(), 0, wxEXPAND);

    auto* rightCol = new wxBoxSizer(wxVERTICAL);
    rightCol->Add(buildHeader(), 0, wxEXPAND);

    book_ = new wxSimplebook(this, wxID_ANY);
    book_->SetBackgroundColour(styles::Background);

    book_->AddPage(new DashboardWidget(book_), utf8::U("Заказы"));
    book_->AddPage(new wxPanel(book_), utf8::U("Склад"));
    book_->AddPage(new wxPanel(book_), utf8::U("Отчёты"));
    book_->AddPage(new wxPanel(book_), utf8::U("Администрирование"));

    rightCol->Add(book_, 1, wxEXPAND);
    root->Add(rightCol, 1, wxEXPAND);

    SetSizer(root);
    Centre();
    setActiveNav(1001);
}

wxPanel* MainWindow::buildNavItem(wxWindow* parent, const wxString& label,
                                  const wxString& iconPath, int id, bool active)
{
    auto* panel = new wxPanel(parent, id, wxDefaultPosition, wxSize(-1, 48));
    panel->SetBackgroundColour(active ? styles::Primary : styles::Sidebar);
    panel->SetCursor(wxCursor(wxCURSOR_HAND));

    auto* sizer = new wxBoxSizer(wxHORIZONTAL);

    auto* bmp = new wxStaticBitmap(panel, wxID_ANY, loadWhiteIcon(iconPath, 20));
    sizer->Add(bmp, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 16);

    auto* txt = new wxStaticText(panel, wxID_ANY, label);
    txt->SetForegroundColour(*wxWHITE);
    txt->SetFont(wxFont(13, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    sizer->Add(txt, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 12);

    panel->SetSizer(sizer);

    panel->Bind(wxEVT_LEFT_UP, [this, id](wxMouseEvent&) {
        wxCommandEvent evt(wxEVT_BUTTON, id);
        onNavClick(evt);
    });

    return panel;
}

wxPanel* MainWindow::buildSidebar() {
    auto* sidebar = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(270, -1));
    sidebar->SetBackgroundColour(styles::Sidebar);

    auto* sizer = new wxBoxSizer(wxVERTICAL);

    auto* logoRow = new wxBoxSizer(wxHORIZONTAL);

    auto* logoBox = new wxPanel(sidebar, wxID_ANY, wxDefaultPosition, wxSize(42, 42));
    logoBox->SetBackgroundColour(styles::Primary);

    auto* logoSizer = new wxBoxSizer(wxVERTICAL);
    auto* logoBmp = new wxStaticBitmap(logoBox, wxID_ANY,
                                       loadWhiteIcon("resources/icons/cpu.png", 24));
    logoSizer->AddStretchSpacer();
    logoSizer->Add(logoBmp, 0, wxALIGN_CENTER);
    logoSizer->AddStretchSpacer();
    logoBox->SetSizer(logoSizer);

    auto* nameCol = new wxBoxSizer(wxVERTICAL);
    auto* name = new wxStaticText(sidebar, wxID_ANY, utf8::U("Ремонт-Учёт"));
    name->SetForegroundColour(*wxWHITE);
    name->SetFont(wxFont(15, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    auto* version = new wxStaticText(sidebar, wxID_ANY, "v1.0.0");
    version->SetForegroundColour(wxColour(0x94, 0xA3, 0xB8));
    version->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    nameCol->Add(name);
    nameCol->Add(version);

    logoRow->Add(logoBox, 0, wxALIGN_CENTER_VERTICAL);
    logoRow->AddSpacer(12);
    logoRow->Add(nameCol, 1, wxALIGN_CENTER_VERTICAL);

    sizer->Add(logoRow, 0, wxEXPAND | wxALL, 20);
    sizer->AddSpacer(8);

    navOrders_    = buildNavItem(sidebar, utf8::U("Заказы"),
                                 "resources/icons/nav_orders.png", 1001, true);
    navWarehouse_ = buildNavItem(sidebar, utf8::U("Склад"),
                                 "resources/icons/nav_warehouse.png", 1002, false);
    navReports_   = buildNavItem(sidebar, utf8::U("Отчёты"),
                                 "resources/icons/nav_reports.png", 1003, false);
    navAdmin_     = buildNavItem(sidebar, utf8::U("Администрирование"),
                                 "resources/icons/nav_admin.png", 1004, false);

    sizer->Add(navOrders_, 0, wxEXPAND | wxLEFT | wxRIGHT, 12);
    sizer->AddSpacer(4);
    sizer->Add(navWarehouse_, 0, wxEXPAND | wxLEFT | wxRIGHT, 12);
    sizer->AddSpacer(4);
    sizer->Add(navReports_, 0, wxEXPAND | wxLEFT | wxRIGHT, 12);
    sizer->AddSpacer(4);
    sizer->Add(navAdmin_, 0, wxEXPAND | wxLEFT | wxRIGHT, 12);

    sizer->AddStretchSpacer(1);

    auto* sep = new wxPanel(sidebar, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
    sep->SetBackgroundColour(wxColour(0x33, 0x41, 0x55));
    sizer->Add(sep, 0, wxEXPAND | wxLEFT | wxRIGHT, 16);
    sizer->AddSpacer(12);

    auto* profileRow = new wxBoxSizer(wxHORIZONTAL);

    auto* avatar = new wxPanel(sidebar, wxID_ANY, wxDefaultPosition, wxSize(42, 42));
    avatar->SetBackgroundColour(wxColour(0x47, 0x55, 0x69));
    auto* avatarTxt = new wxStaticText(avatar, wxID_ANY, "AD",
                                       wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
    avatarTxt->SetForegroundColour(*wxWHITE);
    avatarTxt->SetFont(wxFont(13, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    auto* avSizer = new wxBoxSizer(wxVERTICAL);
    avSizer->AddStretchSpacer();
    avSizer->Add(avatarTxt, 0, wxALIGN_CENTER);
    avSizer->AddStretchSpacer();
    avatar->SetSizer(avSizer);

    auto* profCol = new wxBoxSizer(wxVERTICAL);
    auto* profRole = new wxStaticText(sidebar, wxID_ANY, utf8::U("Администратор"));
    profRole->SetForegroundColour(*wxWHITE);
    profRole->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    auto* profLogin = new wxStaticText(sidebar, wxID_ANY,
                                       wxString::FromUTF8(user_.login));
    profLogin->SetForegroundColour(wxColour(0x94, 0xA3, 0xB8));
    profLogin->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    profCol->Add(profRole);
    profCol->Add(profLogin);

    auto* gear = new wxStaticText(sidebar, wxID_ANY, "⚙");
    gear->SetForegroundColour(wxColour(0x94, 0xA3, 0xB8));
    gear->SetFont(wxFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

    profileRow->Add(avatar, 0, wxALIGN_CENTER_VERTICAL);
    profileRow->AddSpacer(10);
    profileRow->Add(profCol, 1, wxALIGN_CENTER_VERTICAL);
    profileRow->Add(gear, 0, wxALIGN_CENTER_VERTICAL);

    sizer->Add(profileRow, 0, wxEXPAND | wxLEFT | wxRIGHT, 16);
    sizer->AddSpacer(8);

    auto* logoutPanel = new wxPanel(sidebar, wxID_ANY, wxDefaultPosition, wxSize(-1, 44));
    logoutPanel->SetBackgroundColour(styles::Sidebar);
    logoutPanel->SetCursor(wxCursor(wxCURSOR_HAND));

    auto* logoutSizer = new wxBoxSizer(wxHORIZONTAL);

    auto* logoutBmp = new wxStaticBitmap(logoutPanel, wxID_ANY,
                                         loadWhiteIcon("resources/icons/nav_logout.png", 18));
    logoutSizer->Add(logoutBmp, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 16);

    auto* logoutTxt = new wxStaticText(logoutPanel, wxID_ANY, utf8::U("Выйти"));
    logoutTxt->SetForegroundColour(wxColour(0x94, 0xA3, 0xB8));
    logoutTxt->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    logoutSizer->Add(logoutTxt, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);

    logoutPanel->SetSizer(logoutSizer);
    logoutPanel->Bind(wxEVT_LEFT_UP, &MainWindow::onLogout, this);
    sizer->Add(logoutPanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 16);

    sidebar->SetSizer(sizer);
    return sidebar;
}

wxPanel* MainWindow::buildHeader() {
    auto* header = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 72));
    header->SetBackgroundColour(styles::Surface);

    auto* sizer = new wxBoxSizer(wxHORIZONTAL);

    headerTitle_ = new wxStaticText(header, wxID_ANY, utf8::U("Заказы"));
    headerTitle_->SetFont(wxFont(22, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    headerTitle_->SetForegroundColour(styles::Text);

    std::time_t t = std::time(nullptr);
    std::tm* tm = std::localtime(&t);
    char dateBuf[32];
    std::strftime(dateBuf, sizeof(dateBuf), "%d.%m.%Y", tm);

    auto* dateLabel = new wxStaticText(header, wxID_ANY, wxString::FromUTF8(dateBuf));
    dateLabel->SetForegroundColour(styles::Muted);
    dateLabel->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

    auto* statusLabel = new wxStaticText(header, wxID_ANY, utf8::U("● Система работает"));
    statusLabel->SetForegroundColour(styles::Success);
    statusLabel->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

    sizer->Add(headerTitle_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 24);
    sizer->AddStretchSpacer(1);
    sizer->Add(dateLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);
    sizer->Add(statusLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 24);

    header->SetSizer(sizer);
    return header;
}

void MainWindow::setActiveNav(int id) {
    wxColour active = styles::Primary;
    wxColour normal = styles::Sidebar;
    navOrders_->SetBackgroundColour(id == 1001 ? active : normal);
    navWarehouse_->SetBackgroundColour(id == 1002 ? active : normal);
    navReports_->SetBackgroundColour(id == 1003 ? active : normal);
    navAdmin_->SetBackgroundColour(id == 1004 ? active : normal);
    navOrders_->Refresh();
    navWarehouse_->Refresh();
    navReports_->Refresh();
    navAdmin_->Refresh();
}

void MainWindow::onNavClick(wxCommandEvent& event) {
    int id = event.GetId();
    if (id == 1001) { book_->SetSelection(0); headerTitle_->SetLabel(utf8::U("Заказы")); }
    else if (id == 1002) { book_->SetSelection(1); headerTitle_->SetLabel(utf8::U("Склад")); }
    else if (id == 1003) { book_->SetSelection(2); headerTitle_->SetLabel(utf8::U("Отчёты")); }
    else if (id == 1004) { book_->SetSelection(3); headerTitle_->SetLabel(utf8::U("Администрирование")); }
    setActiveNav(id);
}

void MainWindow::onLogout(wxEvent&) {
    Close();
}

}