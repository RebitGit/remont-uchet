#include "ui/MainWindow.h"
#include "ui/DashboardWidget.h"
#include "ui/RoundedPanel.h"
#include "resources/styles.h"
#include "resources/utf8.h"

#include <wx/statbmp.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <ctime>

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
        gc->SetBrush(wxBrush(styles::Primary));
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

    Bind(wxEVT_CLOSE_WINDOW, &MainWindow::onClose, this);
}

MainWindow::~MainWindow() = default;

RoundedPanel* MainWindow::buildNavItem(wxWindow* parent, const wxString& label,
                                       const wxString& iconPath, int id, bool active)
{
    auto* panel = new RoundedPanel(parent,
                                   active ? styles::Primary : styles::Sidebar,
                                   8, wxSize(-1, 48));
    panel->SetCursor(wxCursor(wxCURSOR_HAND));
    panel->SetHoverColour(styles::SidebarHover);
    panel->SetIcon(loadWhiteIcon(iconPath, 20));
    panel->SetLabel(label, *wxWHITE, 13);
    panel->SetActive(active);

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

    auto* logoBox = new LogoBox(sidebar, 42);

    auto* nameCol = new wxBoxSizer(wxVERTICAL);
    auto* name = new wxStaticText(sidebar, wxID_ANY, utf8::U("Ремонт-Учёт"));
    name->SetForegroundColour(*wxWHITE);
    name->SetBackgroundColour(styles::Sidebar);
    name->SetFont(wxFont(15, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    auto* version = new wxStaticText(sidebar, wxID_ANY, "v1.0.0");
    version->SetForegroundColour(wxColour(0x94, 0xA3, 0xB8));
    version->SetBackgroundColour(styles::Sidebar);
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

    auto* avatar = new CircleAvatar(sidebar, "AD",
                                    wxColour(0x47, 0x55, 0x69),
                                    *wxWHITE, 42);

    auto* profCol = new wxBoxSizer(wxVERTICAL);
    auto* profRole = new wxStaticText(sidebar, wxID_ANY, utf8::U("Администратор"));
    profRole->SetForegroundColour(*wxWHITE);
    profRole->SetBackgroundColour(styles::Sidebar);
    profRole->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    auto* profLogin = new wxStaticText(sidebar, wxID_ANY,
                                       wxString::FromUTF8(user_.login));
    profLogin->SetForegroundColour(wxColour(0x94, 0xA3, 0xB8));
    profLogin->SetBackgroundColour(styles::Sidebar);
    profLogin->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    profCol->Add(profRole);
    profCol->Add(profLogin);

    auto* gearPanel = new RoundedPanel(sidebar, styles::Sidebar, 15, wxSize(30, 30));
    gearPanel->SetCursor(wxCursor(wxCURSOR_HAND));
    gearPanel->SetHoverColour(styles::SidebarHover);
    gearPanel->SetIcon(loadWhiteIcon("resources/icons/settings.png", 18));

    profileRow->Add(avatar, 0, wxALIGN_CENTER_VERTICAL);
    profileRow->AddSpacer(10);
    profileRow->Add(profCol, 1, wxALIGN_CENTER_VERTICAL);
    profileRow->Add(gearPanel, 0, wxALIGN_CENTER_VERTICAL);

    sizer->Add(profileRow, 0, wxEXPAND | wxLEFT | wxRIGHT, 16);
    sizer->AddSpacer(8);

    auto* logoutPanel = new RoundedPanel(sidebar, styles::Sidebar, 8, wxSize(-1, 44));
    logoutPanel->SetCursor(wxCursor(wxCURSOR_HAND));
    logoutPanel->SetHoverColour(styles::SidebarHover);
    logoutPanel->SetIcon(loadWhiteIcon("resources/icons/nav_logout.png", 18));
    logoutPanel->SetLabel(utf8::U("Выйти"), wxColour(0x94, 0xA3, 0xB8), 12);

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
    if (id == 1001) { book_->SetSelection(0); headerTitle_->SetLabel(utf8::U("Заказы")); }
    else if (id == 1002) { book_->SetSelection(1); headerTitle_->SetLabel(utf8::U("Склад")); }
    else if (id == 1003) { book_->SetSelection(2); headerTitle_->SetLabel(utf8::U("Отчёты")); }
    else if (id == 1004) { book_->SetSelection(3); headerTitle_->SetLabel(utf8::U("Администрирование")); }
    setActiveNav(id);
}

void MainWindow::onLogout(wxEvent&) {
    logoutRequested_ = true;
    Close();
}

void MainWindow::onClose(wxCloseEvent& event) {
    event.Skip();
}

}