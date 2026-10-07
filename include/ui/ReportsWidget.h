#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>

namespace remont {

class ReportsWidget : public wxPanel {
public:
    ReportsWidget(wxWindow* parent);

private:
    void buildToolbar(wxSizer* root);
    void buildTable(wxSizer* root);
    void buildSummary(wxSizer* root);
    void loadReport();

    void onReportTypeChanged(wxCommandEvent& event);
    void onPeriodChanged(wxCommandEvent& event);
    void onExport(wxCommandEvent& event);

    void loadStatusReport();
    void loadPartsReport();
    void loadMastersReport();

    wxSizer* mainSizer_ = nullptr;
    wxChoice* typeChoice_ = nullptr;
    wxChoice* periodChoice_ = nullptr;
    wxListCtrl* table_ = nullptr;
    wxStaticText* summaryLabel_ = nullptr;

    int reportType_ = 0;
    int period_ = 1;
};

}