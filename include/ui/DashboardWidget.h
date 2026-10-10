#pragma once
#include <wx/wx.h>
#include <wx/scrolwin.h>
#include <functional>
#include <vector>
#include "models/User.h"
#include "models/Order.h"
#include "models/Client.h"
#include "models/Device.h"
#include "utils/TableExport.h"

namespace remont {

class DashboardWidget : public wxPanel {
public:
    DashboardWidget(wxWindow* parent, const User& user, int onlyMasterId = 0);

    void setOnNewOrder(std::function<void()> cb) { onNewOrder_ = cb; }
    void setOnViewOrder(std::function<void(int)> cb) { onViewOrder_ = cb; }
    void setOnPrintOrder(std::function<void(int)> cb) { onPrintOrder_ = cb; }
    void reload();

private:
    void buildStats(wxSizer* root);
    void buildToolbar(wxSizer* root);
    void buildHeader(wxSizer* root, wxWindow* parent);
    void buildList(wxSizer* root);
    void loadOrders();
    void applyFilter();
    wxPanel* createRow(wxWindow* parent, const Order& o, const Client& c, const Device& d);

    void onExport(wxCommandEvent& event);
    void onPrint(wxCommandEvent& event);
    TableData buildTableData();

    User user_;
    int onlyMasterId_ = 0;

    wxScrolledWindow* list_ = nullptr;
    wxPanel* header_ = nullptr;
    wxTextCtrl* search_ = nullptr;
    wxChoice* statusFilter_ = nullptr;
    std::function<void()> onNewOrder_;
    std::function<void(int)> onViewOrder_;
    std::function<void(int)> onPrintOrder_;
};

}