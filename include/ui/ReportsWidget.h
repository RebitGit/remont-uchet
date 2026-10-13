#pragma once

#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <wx/choice.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/datectrl.h>
#include <wx/dateevt.h>

#include <vector>
#include <string>

#include "utils/TableExport.h"

namespace remont {

struct StatusStat {
    std::string name;
    int count = 0;
    wxColour color;
};

class BarChartPanel : public wxPanel {
public:
    explicit BarChartPanel(wxWindow* parent);
    void setData(const std::vector<StatusStat>& data);

private:
    void onPaint(wxPaintEvent& event);

    std::vector<StatusStat> data_;
};

class DonutChartPanel : public wxPanel {
public:
    explicit DonutChartPanel(wxWindow* parent);
    void setData(const std::vector<StatusStat>& data, int total);

private:
    void onPaint(wxPaintEvent& event);

    std::vector<StatusStat> data_;
    int total_ = 0;
};

class StatsTablePanel : public wxPanel {
public:
    explicit StatsTablePanel(wxWindow* parent);
    void setData(const std::vector<StatusStat>& data, int total);

private:
    void onPaint(wxPaintEvent& event);

    std::vector<StatusStat> data_;
    int total_ = 0;
};

class ReportsWidget : public wxPanel {
public:
    explicit ReportsWidget(wxWindow* parent);
    void reload();

private:
    void buildToolbar(wxSizer* root);
    void buildStatCards(wxSizer* root);
    void buildCharts(wxSizer* root);
    void buildTable(wxSizer* root);

    void loadReport();
    TableData buildTableData();

    void onExport(wxCommandEvent& event);
    void onPrint(wxCommandEvent& event);
    void onPeriodChanged(wxCommandEvent& event);
    void onCustomDateChanged(wxDateEvent& event);

    wxScrolledWindow* scroll_ = nullptr;

    wxChoice* periodChoice_ = nullptr;
    wxChoice* typeChoice_   = nullptr;

    wxPanel* customRow_ = nullptr;

    wxStaticText*     customFromLabel_ = nullptr;
    wxDatePickerCtrl* customFromDate_  = nullptr;
    wxStaticText*     customToLabel_   = nullptr;
    wxDatePickerCtrl* customToDate_    = nullptr;

    wxStaticText* totalOrdersValue_ = nullptr;
    wxStaticText* workCostValue_    = nullptr;
    wxStaticText* partsCostValue_   = nullptr;
    wxStaticText* totalCostValue_   = nullptr;

    wxStaticText* legendAcceptedPct_ = nullptr;
    wxStaticText* legendDiagPct_     = nullptr;
    wxStaticText* legendRepairPct_   = nullptr;
    wxStaticText* legendWaitingPct_  = nullptr;
    wxStaticText* legendReadyPct_    = nullptr;
    wxStaticText* legendIssuedPct_   = nullptr;

    BarChartPanel*   barChart_   = nullptr;
    DonutChartPanel* donutChart_ = nullptr;
    StatsTablePanel* statsTable_ = nullptr;

    std::vector<StatusStat> lastStats_;
    int lastTotal_ = 0;
};

}