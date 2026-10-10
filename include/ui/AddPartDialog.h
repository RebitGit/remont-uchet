#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <vector>
#include "models/Part.h"

namespace remont {

class AddPartDialog : public wxDialog {
public:
    AddPartDialog(wxWindow* parent);

    int getSelectedPartId() const { return selectedPartId_; }
    int getQuantity() const { return quantity_; }

private:
    void loadParts();
    void onPartSelected(wxListEvent& event);
    void onOk(wxCommandEvent& event);
    void onCancel(wxCommandEvent& event);

    wxListCtrl* partsTable_ = nullptr;
    wxTextCtrl* qtyField_ = nullptr;
    wxStaticText* errorLabel_ = nullptr;
    wxPanel* errorPanel_ = nullptr;

    int selectedPartId_ = 0;
    int quantity_ = 0;
    std::vector<Part> parts_;
};

}