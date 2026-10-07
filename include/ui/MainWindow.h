#pragma once
#include <wx/wx.h>
#include <wx/simplebook.h>
#include "models/User.h"
#include "ui/RoundedPanel.h"

namespace remont {

class NewOrderWidget;
class DashboardWidget;
class WarehouseWidget;

class MainWindow : public wxFrame {
public:
    MainWindow(const User& user);
    ~MainWindow();

    bool logoutRequested() const { return logoutRequested_; }
    bool themeChanged() const { return themeChanged_; }

private:
    wxPanel* buildSidebar();
    wxPanel* buildHeader();
    RoundedPanel* buildNavItem(wxWindow* parent, const wxString& label,
                               const wxString& iconPath, int id, bool active);

    void onNavClick(wxCommandEvent& event);
    void onLogout(wxEvent& event);
    void onSettingsClick(wxMouseEvent& event);
    void onClose(wxCloseEvent& event);
    void onBackToDashboard(wxCommandEvent& event);
    void setActiveNav(int id);
    void showNewOrderPage();

    User user_;
    wxSimplebook* book_ = nullptr;

    wxPanel* sidebar_ = nullptr;
    wxPanel* header_ = nullptr;
    wxStaticText* headerTitle_ = nullptr;

    RoundedPanel* navOrders_ = nullptr;
    RoundedPanel* navWarehouse_ = nullptr;
    RoundedPanel* navReports_ = nullptr;
    RoundedPanel* navAdmin_ = nullptr;

    DashboardWidget* dashboardPage_ = nullptr;
    NewOrderWidget* newOrderPage_ = nullptr;

    bool logoutRequested_ = false;
    bool themeChanged_ = false;
    bool closeHandled_ = false;
    int currentNavId_ = 1001;
};

}