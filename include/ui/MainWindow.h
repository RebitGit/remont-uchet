#pragma once
#include <wx/wx.h>
#include <wx/simplebook.h>
#include "models/User.h"

namespace remont {

class MainWindow : public wxFrame {
public:
    MainWindow(const User& user);

private:
    wxPanel* buildSidebar();
    wxPanel* buildHeader();
    wxPanel* buildNavItem(wxWindow* parent, const wxString& label,
                          const wxString& iconPath, int id, bool active);

    void onNavClick(wxCommandEvent& event);
    void onLogout(wxEvent& event);
    void setActiveNav(int id);

    User user_;
    wxSimplebook* book_ = nullptr;

    wxPanel* navOrders_ = nullptr;
    wxPanel* navWarehouse_ = nullptr;
    wxPanel* navReports_ = nullptr;
    wxPanel* navAdmin_ = nullptr;
    wxStaticText* headerTitle_ = nullptr;
};

}