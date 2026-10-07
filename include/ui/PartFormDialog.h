#pragma once
#include <wx/wx.h>
#include "models/Part.h"

namespace remont {

class PartFormDialog : public wxDialog {
public:
    PartFormDialog(wxWindow* parent, const Part& part = Part());
    Part getPart() const { return part_; }

private:
    void onSave(wxCommandEvent& event);
    void onCancel(wxCommandEvent& event);

    wxTextCtrl* articleField_ = nullptr;
    wxTextCtrl* priceField_ = nullptr;
    wxTextCtrl* nameField_ = nullptr;
    wxTextCtrl* qtyField_ = nullptr;
    wxTextCtrl* minQtyField_ = nullptr;
    wxStaticText* errorLabel_ = nullptr;
    wxPanel* errorPanel_ = nullptr;

    Part part_;
    bool isEdit_ = false;
};

}