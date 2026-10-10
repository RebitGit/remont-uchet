#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include "models/User.h"
#include "models/Order.h"

namespace remont {

class OrderViewWidget : public wxPanel {
public:
    OrderViewWidget(wxWindow* parent, const User& user);

    void setOrderId(int orderId);
    void reload();

private:
    wxPanel* buildBreadcrumb(wxSizer* root, wxWindow* parent);
    wxPanel* buildHeader(wxSizer* root, wxWindow* parent);
    wxPanel* buildInfoCard(wxWindow* parent);
    wxPanel* buildPartsCard(wxWindow* parent);
    wxPanel* buildStatusCard(wxWindow* parent);
    wxPanel* buildHistoryCard(wxWindow* parent);
    wxSizer* buildTotalsRow(wxWindow* parent);

    void loadOrder();

    void onBack(wxMouseEvent& event);
    void onPrint(wxCommandEvent& event);
    void onAddPart(wxCommandEvent& event);
    void onChangeStatus(wxCommandEvent& event);

    User user_;
    int orderId_ = 0;
    Order order_;

    wxStaticText* titleLabel_ = nullptr;
    wxStaticText* statusBadge_ = nullptr;
    wxStaticText* clientName_ = nullptr;
    wxStaticText* clientPhone_ = nullptr;
    wxStaticText* deviceModel_ = nullptr;
    wxStaticText* deviceSerial_ = nullptr;
    wxStaticText* description_ = nullptr;
    wxStaticText* receivedAt_ = nullptr;
    wxStaticText* completedAt_ = nullptr;
    wxStaticText* master_ = nullptr;
    wxStaticText* totalCost_ = nullptr;

    wxListCtrl* partsTable_ = nullptr;
    wxChoice* statusChoice_ = nullptr;
    wxPanel* historyPanel_ = nullptr;

    wxStaticText* partsTotal_ = nullptr;
    wxStaticText* workCost_ = nullptr;
    wxStaticText* grandTotal_ = nullptr;
};

}