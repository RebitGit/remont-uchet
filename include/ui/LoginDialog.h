#pragma once
#include <wx/wx.h>
#include <wx/textctrl.h>
#include <wx/statbmp.h>
#include <wx/checkbox.h>
#include "models/User.h"

namespace remont {

class LoginDialog : public wxDialog {
public:
    LoginDialog(wxWindow* parent);
    wxString getLogin() const;
    wxString getPassword() const;
    User getUser() const { return user_; }

private:
    wxTextCtrl* loginField_ = nullptr;
    wxTextCtrl* passwordField_ = nullptr;
    wxTextCtrl* passwordPlain_ = nullptr;
    wxStaticText* errorLabel_ = nullptr;
    wxPanel* errorPanel_ = nullptr;
    wxButton* loginButton_ = nullptr;
    wxCheckBox* rememberCheck_ = nullptr;
    wxStaticBitmap* eyeIcon_ = nullptr;

    wxBitmap eyeOpen_;
    wxBitmap eyeClosed_;
    bool passwordVisible_ = false;
    User user_;

    void onLogin(wxCommandEvent& event);
    void onTogglePassword(wxMouseEvent& event);
    void showError(const wxString& text);
    void hideError();
};

}