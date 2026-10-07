#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include "ui/ClickableListCtrl.h"
#include "models/User.h"
#include <vector>

namespace remont {

class AdminWidget : public wxPanel {
public:
    AdminWidget(wxWindow* parent);

private:
    void buildToolbar(wxSizer* root);
    void buildTable(wxSizer* root);
    void buildFooter(wxSizer* root);
    void loadUsers();
    void applyFilter();

    void onAddUser(wxCommandEvent& event);
    void onBackup(wxCommandEvent& event);
    void onTableClick(int row, int col);

    void toggleActive(int userId);
    void changeRole(int userId);
    void resetPassword(int userId);

    wxSizer* mainSizer_ = nullptr;
    ClickableListCtrl* table_ = nullptr;
    wxStaticText* footerCount_ = nullptr;

    std::vector<User> allUsers_;
};

}