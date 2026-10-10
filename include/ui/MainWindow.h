#pragma once
#include <wx/wx.h>
#include <wx/simplebook.h>
#include <wx/timer.h>
#include "models/User.h"
#include "ui/RoundedPanel.h"

namespace remont {

class NewOrderWidget;
class DashboardWidget;
class WarehouseWidget;
class OrderViewWidget;
class ReportsWidget;
class AdminWidget;

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
    void onShowOrderView(wxCommandEvent& event);
    void setActiveNav(int id);
    void showNewOrderPage();
    void showOrderViewPage(int orderId);
    void printOrder(int orderId);

    void onBackupTimer(wxTimerEvent& event);
    void tryAutoBackup();

    void selectPage(wxWindow* page, const wxString& title, int navId);

    User user_;
    wxSimplebook* book_ = nullptr;

    wxPanel* sidebar_ = nullptr;
    wxPanel* header_ = nullptr;
    wxStaticText* headerTitle_ = nullptr;

    RoundedPanel* navOrders_ = nullptr;
    RoundedPanel* navMyOrders_ = nullptr;
    RoundedPanel* navWarehouse_ = nullptr;
    RoundedPanel* navReports_ = nullptr;
    RoundedPanel* navAdmin_ = nullptr;

    DashboardWidget* dashboardPage_ = nullptr;
    DashboardWidget* myOrdersPage_ = nullptr;
    WarehouseWidget* warehousePage_ = nullptr;
    ReportsWidget* reportsPage_ = nullptr;
    AdminWidget* adminPage_ = nullptr;
    NewOrderWidget* newOrderPage_ = nullptr;
    OrderViewWidget* orderViewPage_ = nullptr;

    wxTimer backupTimer_;

    bool logoutRequested_ = false;
    bool themeChanged_ = false;
    bool closeHandled_ = false;
    int currentNavId_ = 1001;
};

}