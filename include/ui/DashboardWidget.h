#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <functional>

namespace remont {

class DashboardWidget : public wxPanel {
public:
    DashboardWidget(wxWindow* parent);

    void setOnNewOrder(std::function<void()> cb) { onNewOrder_ = cb; }
    void reload();

private:
    void buildStats(wxSizer* root);
    void buildToolbar(wxSizer* root);
    void buildTable(wxSizer* root);
    void loadOrders();

    wxListCtrl* table_ = nullptr;
    wxTextCtrl* search_ = nullptr;
    std::function<void()> onNewOrder_;
};

}