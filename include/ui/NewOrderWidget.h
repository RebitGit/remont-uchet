#pragma once
#include <wx/wx.h>
#include "models/User.h"
#include <vector>

namespace remont {

class NewOrderWidget : public wxPanel {
public:
    NewOrderWidget(wxWindow* parent, const User& user);

    void reset();
    void reload();

private:
    wxPanel* buildBreadcrumb(wxSizer* root, wxWindow* parent);
    wxPanel* buildClientCard(wxSizer* root, wxWindow* parent);
    wxPanel* buildDeviceCard(wxSizer* root, wxWindow* parent);
    wxPanel* buildExtraCard(wxSizer* root, wxWindow* parent);
    void buildButtons(wxSizer* root, wxWindow* parent);

    void onCancel(wxCommandEvent& event);
    void onSave(wxCommandEvent& event);
    void onSaveAndPrint(wxCommandEvent& event);

    bool validate();
    bool saveOrder(bool printAfter);

    void loadMasters();

    User user_;

    wxTextCtrl* fioField_ = nullptr;
    wxTextCtrl* phoneField_ = nullptr;
    wxTextCtrl* emailField_ = nullptr;

    wxChoice* deviceTypeChoice_ = nullptr;
    wxTextCtrl* modelField_ = nullptr;
    wxTextCtrl* serialField_ = nullptr;
    wxTextCtrl* descriptionField_ = nullptr;

    wxTextCtrl* costField_ = nullptr;
    wxChoice* masterChoice_ = nullptr;

    wxStaticText* errorLabel_ = nullptr;
    wxPanel* errorPanel_ = nullptr;

    std::vector<int> masterIds_;
};

}