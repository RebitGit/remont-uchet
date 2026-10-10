#pragma once
#include <wx/wx.h>
#include <wx/scrolwin.h>
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
    BarChartPanel(wxWindow* parent);
    void setData(const std::vector<StatusStat>& data);
private:
    void onPaint(wxPaintEvent& event);
    std::vector<StatusStat> data_;
};

class DonutChartPanel : public wxPanel {
public:
    DonutChartPanel(wxWindow* parent);
    void setData(const std::vector<StatusStat>& data, int total);
private:
    void onPaint(wxPaintEvent& event);
    std::vector<StatusStat> data_;
    int total_ = 0;
};

class StatsTablePanel : public wxPanel {
public:
    StatsTablePanel(wxWindow* parent);
    void setData(const std::vector<StatusStat>& data, int total);
private:
    void onPaint(wxPaintEvent& event);
    std::vector<StatusStat> data_;
    int total_ = 0;
};

class ReportsWidget : public wxPanel {
public:
    ReportsWidget(wxWindow* parent);

    void reload();

private:
    void buildToolbar(wxSizer* root);
    void buildStatCards(wxSizer* root);
    void buildCharts(wxSizer* root);
    void buildTable(wxSizer* root);
    void loadReport();

    void onExport(wxCommandEvent& event);
    void onPrint(wxCommandEvent& event);
    TableData buildTableData();

    wxScrolledWindow* scroll_ = nullptr;
    wxChoice* periodChoice_ = nullptr;
    wxChoice* typeChoice_ = nullptr;

    wxStaticText* totalOrdersValue_ = nullptr;
    wxStaticText* revenueValue_ = nullptr;
    wxStaticText* avgTimeValue_ = nullptr;

    BarChartPanel* barChart_ = nullptr;
    DonutChartPanel* donutChart_ = nullptr;
    StatsTablePanel* statsTable_ = nullptr;

    wxStaticText* legendAcceptedPct_ = nullptr;
    wxStaticText* legendDiagPct_ = nullptr;
    wxStaticText* legendRepairPct_ = nullptr;
    wxStaticText* legendWaitingPct_ = nullptr;
    wxStaticText* legendReadyPct_ = nullptr;
    wxStaticText* legendIssuedPct_ = nullptr;

    std::vector<StatusStat> lastStats_;
    int lastTotal_ = 0;
};

}