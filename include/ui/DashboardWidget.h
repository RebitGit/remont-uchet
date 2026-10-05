#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>

namespace remont {

class DashboardWidget : public wxPanel {
public:
    DashboardWidget(wxWindow* parent);

private:
    void buildStats(wxSizer* root);
    void buildToolbar(wxSizer* root);
    void buildTable(wxSizer* root);

    wxListCtrl* table_ = nullptr;
    wxTextCtrl* search_ = nullptr;
};

}