#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/scrolwin.h>
#include <wx/simplebook.h>
#include "models/User.h"
#include <vector>
#include <string>

namespace remont {

class AdminWidget : public wxPanel {
public:
    AdminWidget(wxWindow* parent, const User& currentUser);
    ~AdminWidget();

private:
    wxPanel* buildTabBar();
    wxPanel* buildTabItem(wxWindow* parent, const wxString& label, int index);
    void switchTab(int index);

    wxPanel* buildUsersPage(wxWindow* parent);
    wxPanel* buildRolesPage(wxWindow* parent);
    wxPanel* buildSettingsPage(wxWindow* parent);
    wxPanel* buildBackupPage(wxWindow* parent);

    void loadUsers();
    void refreshUsersList();
    wxPanel* createUserRow(wxWindow* parent, const User& u);
    wxPanel* createColumnHeader(wxWindow* parent);

    void onAddUser(wxCommandEvent& event);
    void onEditUser(int userId);
    void onResetPassword(int userId);
    void onToggleActive(int userId);
    void onDeleteUser(int userId);
    void onBackup(wxCommandEvent& event);

    void loadBackups();
    void rebuildBackupRows();

    User currentUser_;
    std::vector<User> allUsers_;

    wxPanel* tabBar_ = nullptr;
    std::vector<wxPanel*> tabItems_;
    std::vector<wxStaticText*> tabLabels_;
    std::vector<wxPanel*> tabUnderlines_;
    int activeTab_ = 0;

    wxSimplebook* pages_ = nullptr;

    wxScrolledWindow* usersList_ = nullptr;
    wxPanel* usersCard_ = nullptr;

    wxScrolledWindow* backupsList_ = nullptr;
    wxPanel* backupsCard_ = nullptr;

    struct BackupEntry {
        std::string fileName;
        double sizeMb = 0.0;
        std::string dateStr;
        bool isAuto = false;
    };
    std::vector<BackupEntry> backups_;
};

}