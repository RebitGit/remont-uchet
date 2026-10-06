#pragma once
#include <wx/wx.h>
#include <wx/simplebook.h>
#include "models/User.h"
#include "ui/RoundedPanel.h"

namespace remont {

class MainWindow : public wxFrame {
public:
    MainWindow(const User& user);
    ~MainWindow();

    bool logoutRequested() const { return logoutRequested_; }

private:
    wxPanel* buildSidebar();
    wxPanel* buildHeader();
    RoundedPanel* buildNavItem(wxWindow* parent, const wxString& label,
                               const wxString& iconPath, int id, bool active);

    void onNavClick(wxCommandEvent& event);
    void onLogout(wxEvent& event);
    void onClose(wxCloseEvent& event);
    void setActiveNav(int id);

    User user_;
    wxSimplebook* book_ = nullptr;

    RoundedPanel* navOrders_ = nullptr;
    RoundedPanel* navWarehouse_ = nullptr;
    RoundedPanel* navReports_ = nullptr;
    RoundedPanel* navAdmin_ = nullptr;
    wxStaticText* headerTitle_ = nullptr;

    bool logoutRequested_ = false;
    bool closeHandled_ = false;
};

}