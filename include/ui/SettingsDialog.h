#pragma once
#include <wx/wx.h>
#include <wx/scrolwin.h>
#include "models/User.h"

namespace remont {

class SettingsDialog : public wxDialog {
public:
    SettingsDialog(wxWindow* parent, const User& user);

private:
    wxPanel* buildAccountCard(wxWindow* parent);
    wxPanel* buildAppearanceCard(wxWindow* parent);
    wxPanel* buildPasswordCard(wxWindow* parent);

    void onThemeClick(wxMouseEvent& event, int themeIndex);
    void onTextSizeClick(wxMouseEvent& event, int sizeIndex);
    void onChangePassword(wxCommandEvent& event);

    User user_;
    int selectedTheme_ = 0;
    int selectedTextSize_ = 1;

    wxScrolledWindow* scroll_ = nullptr;

    wxPanel* themePanels_[3] = {};
    wxPanel* textSizePanels_[3] = {};

    wxTextCtrl* currentPassField_ = nullptr;
    wxTextCtrl* newPassField_ = nullptr;
    wxTextCtrl* repeatPassField_ = nullptr;
    wxStaticText* messageLabel_ = nullptr;
};

}