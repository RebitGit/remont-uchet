#include "ui/ReportsWidget.h"
#include "core/DatabaseManager.h"
#include "core/ThemeManager.h"
#include "core/Types.h"
#include "core/ConfigManager.h"
#include "resources/styles.h"
#include "resources/utf8.h"

#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>
#include <sqlite3.h>
#include <fstream>
#include <vector>
#include <algorithm>
#include <ctime>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace remont {

namespace {

wxColour colorForStatus(const std::string& s) {
    if (s == "Принят")            return wxColour(0x25, 0x63, 0xEB);
    if (s == "Диагностика")       return wxColour(0x8B, 0x5C, 0xF6);
    if (s == "В ремонте")         return wxColour(0xF5, 0x9E, 0x0B);
    if (s == "Ожидает запчасть")  return wxColour(0xEF, 0x44, 0x44);
    if (s == "Готов")             return wxColour(0x10, 0xB9, 0x81);
    if (s == "Выдан")             return wxColour(0x6B, 0x72, 0x80);
    return wxColour(0x9C, 0xA3, 0xAF);
}

}

BarChartPanel::BarChartPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 340))
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(ThemeManager::instance().surface());
    Bind(wxEVT_PAINT, &BarChartPanel::onPaint, this);
}

void BarChartPanel::setData(const std::vector<StatusStat>& data) {
    data_ = data;
    Refresh();
}

void BarChartPanel::onPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(ThemeManager::instance().surface()));
    dc.Clear();

    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
    if (!gc) return;

    gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);

    wxSize s = GetSize();
    int W = s.GetWidth();
    int H = s.GetHeight();

    if (data_.empty() || W < 80 || H < 80) return;

    int maxCount = 1;
    for (auto& d : data_) if (d.count > maxCount) maxCount = d.count;

    int topPad = 50;
    int bottomPad = 50;
    int leftPad = 30;
    int rightPad = 30;

    int chartH = H - topPad - bottomPad;
    int chartW = W - leftPad - rightPad;
    int baseY = H - bottomPad;

    int n = static_cast<int>(data_.size());
    if (n == 0) return;

    int slotW = chartW / n;
    int barW = static_cast<int>(slotW * 0.5);
    if (barW > 56) barW = 56;
    if (barW < 22) barW = 22;

    gc->SetPen(wxPen(ThemeManager::instance().border(), 1));
    gc->StrokeLine(leftPad, baseY, W - rightPad, baseY);

    for (int i = 0; i < n; ++i) {
        int barH = static_cast<int>(
            (static_cast<double>(data_[i].count) / maxCount) * chartH);
        if (barH < 6) barH = 6;

        int cx = leftPad + slotW * i + slotW / 2;
        int x = cx - barW / 2;
        int yTop = baseY - barH;

        double radius = 5.0;
        if (radius > barW / 2.0) radius = barW / 2.0;
        if (radius > barH / 2.0) radius = barH / 2.0;

        gc->SetBrush(wxBrush(data_[i].color));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawRoundedRectangle(x, yTop, barW, barH, radius);

        gc->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                           wxFONTWEIGHT_NORMAL),
                    ThemeManager::instance().muted());
        wxString val = wxString::Format("%d", data_[i].count);
        double tw, th;
        gc->GetTextExtent(val, &tw, &th);
        gc->DrawText(val, cx - tw / 2.0, yTop - th - 6);

        gc->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                           wxFONTWEIGHT_NORMAL),
                    ThemeManager::instance().muted());
        wxString label = wxString::FromUTF8(data_[i].name.c_str());
        gc->GetTextExtent(label, &tw, &th);
        gc->DrawText(label, cx - tw / 2.0, baseY + 8);
    }
}

DonutChartPanel::DonutChartPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 320))
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(ThemeManager::instance().surface());
    Bind(wxEVT_PAINT, &DonutChartPanel::onPaint, this);
}

void DonutChartPanel::setData(const std::vector<StatusStat>& data, int total) {
    data_ = data;
    total_ = total;
    Refresh();
}

void DonutChartPanel::onPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(ThemeManager::instance().surface()));
    dc.Clear();

    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
    if (!gc) return;

    gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);

    wxSize s = GetSize();
    int W = s.GetWidth();
    int H = s.GetHeight();

    if (total_ <= 0 || W < 50 || H < 50) return;

    int cx = W / 2;
    int cy = H / 2;
    int outerR = std::min(W, H) / 2 - 20;
    int innerR = static_cast<int>(outerR * 0.6);

    double startAngle = -M_PI / 2;

    for (auto& d : data_) {
        if (d.count == 0) continue;
        double sweep = (static_cast<double>(d.count) / total_) * 2 * M_PI;

        wxGraphicsPath path = gc->CreatePath();
        path.MoveToPoint(cx, cy);
        path.AddArc(cx, cy, outerR, startAngle, startAngle + sweep, true);
        path.CloseSubpath();

        gc->SetBrush(wxBrush(d.color));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->FillPath(path);

        startAngle += sweep;
    }

    gc->SetBrush(wxBrush(ThemeManager::instance().surface()));
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->DrawEllipse(cx - innerR, cy - innerR, innerR * 2, innerR * 2);

    gc->SetFont(wxFont(22, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                       wxFONTWEIGHT_BOLD),
                ThemeManager::instance().text());
    wxString totalStr = wxString::Format("%d", total_);
    double tw, th;
    gc->GetTextExtent(totalStr, &tw, &th);
    gc->DrawText(totalStr, cx - tw / 2.0, cy - th - 2);

    gc->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                       wxFONTWEIGHT_NORMAL),
                ThemeManager::instance().muted());
    wxString label = utf8::U("заказов");
    gc->GetTextExtent(label, &tw, &th);
    gc->DrawText(label, cx - tw / 2.0, cy + 4);
}

StatsTablePanel::StatsTablePanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 320))
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(ThemeManager::instance().surface());
    Bind(wxEVT_PAINT, &StatsTablePanel::onPaint, this);
}

void StatsTablePanel::setData(const std::vector<StatusStat>& data, int total) {
    data_ = data;
    total_ = total;
    Refresh();
}

void StatsTablePanel::onPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(ThemeManager::instance().surface()));
    dc.Clear();

    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
    if (!gc) return;

    gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);

    wxSize s = GetSize();
    int W = s.GetWidth();
    int H = s.GetHeight();

    if (W < 100 || H < 60) return;

    auto& tm = ThemeManager::instance();

    int pad = 24;
    int colStatusX  = pad;
    int colStatusW  = static_cast<int>(W * 0.40);
    int colQtyX     = pad + colStatusW;
    int colQtyW     = static_cast<int>(W * 0.20);
    int colShareX   = colQtyX + colQtyW;
    int colShareW   = W - colShareX - pad;

    int headerH = 44;
    int rowH = 40;

    gc->SetBrush(wxBrush(wxColour(0xF9, 0xFA, 0xFB)));
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->DrawRectangle(0, 0, W, headerH);

    gc->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                       wxFONTWEIGHT_BOLD), tm.muted());

    double tw, th;
    wxString hdrStatus = utf8::U("СТАТУС");
    gc->GetTextExtent(hdrStatus, &tw, &th);
    gc->DrawText(hdrStatus, colStatusX, (headerH - th) / 2.0);

    wxString hdrQty = utf8::U("КОЛИЧЕСТВО");
    gc->GetTextExtent(hdrQty, &tw, &th);
    gc->DrawText(hdrQty, colQtyX + colQtyW - tw - 16, (headerH - th) / 2.0);

    wxString hdrShare = utf8::U("ДОЛЯ");
    gc->GetTextExtent(hdrShare, &tw, &th);
    gc->DrawText(hdrShare, colShareX + colShareW - tw - 16, (headerH - th) / 2.0);

    gc->SetPen(wxPen(tm.border(), 1));
    gc->StrokeLine(0, headerH, W, headerH);

    if (total_ <= 0) return;

    for (size_t i = 0; i < data_.size(); ++i) {
        int y = headerH + rowH * static_cast<int>(i);
        int cy = y + rowH / 2;
        auto& st = data_[i];

        if (i > 0) {
            gc->SetPen(wxPen(tm.border(), 1));
            gc->StrokeLine(0, y, W, y);
        }

        int sqSize = 10;
        int sqX = colStatusX + 4;
        int sqY = cy - sqSize / 2;

        gc->SetBrush(wxBrush(st.color));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawRectangle(sqX, sqY, sqSize, sqSize);

        gc->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                           wxFONTWEIGHT_NORMAL), tm.text());
        wxString name = utf8::U(st.name.c_str());
        gc->GetTextExtent(name, &tw, &th);
        gc->DrawText(name, sqX + sqSize + 12, cy - th / 2.0);

        gc->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                           wxFONTWEIGHT_NORMAL), tm.text());
        wxString qtyStr = wxString::Format("%d", st.count);
        gc->GetTextExtent(qtyStr, &tw, &th);
        gc->DrawText(qtyStr, colQtyX + colQtyW - tw - 16, cy - th / 2.0);

        int pct = total_ > 0
            ? static_cast<int>(std::round(st.count * 100.0 / total_))
            : 0;

        wxString pctStr = wxString::Format("%d%%", pct);
        gc->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                           wxFONTWEIGHT_NORMAL), tm.muted());
        gc->GetTextExtent(pctStr, &tw, &th);
        int pctX = colShareX + colShareW - tw - 16;
        gc->DrawText(pctStr, pctX, cy - th / 2.0);

        int barX = colShareX + 8;
        int barW = pctX - barX - 12;
        if (barW < 20) barW = 20;
        int barH = 6;
        int barY = cy - barH / 2;

        gc->SetBrush(wxBrush(tm.border()));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawRoundedRectangle(barX, barY, barW, barH, barH / 2.0);

        int fillW = static_cast<int>(barW * (pct / 100.0));
        if (fillW > 0) {
            gc->SetBrush(wxBrush(st.color));
            gc->DrawRoundedRectangle(barX, barY, fillW, barH, barH / 2.0);
        }
    }

    int bottomY = headerH + rowH * static_cast<int>(data_.size());
    gc->SetPen(wxPen(tm.border(), 1));
    gc->StrokeLine(0, bottomY, W, bottomY);
}

ReportsWidget::ReportsWidget(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
{
    auto& tm = ThemeManager::instance();
    SetBackgroundColour(tm.background());

    auto* outer = new wxBoxSizer(wxVERTICAL);

    scroll_ = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition,
                                   wxDefaultSize, wxVSCROLL | wxBORDER_NONE);
    scroll_->SetBackgroundColour(tm.background());
    scroll_->SetScrollRate(0, 16);

    auto* root = new wxBoxSizer(wxVERTICAL);
    root->AddSpacer(16);

    buildToolbar(root);
    root->AddSpacer(16);
    buildStatCards(root);
    root->AddSpacer(16);
    buildCharts(root);
    root->AddSpacer(16);
    buildTable(root);
    root->AddSpacer(20);

    scroll_->SetSizer(root);
    scroll_->FitInside();

    outer->Add(scroll_, 1, wxEXPAND);
    SetSizer(outer);

    loadReport();
}

void ReportsWidget::reload() {
    loadReport();
}

void ReportsWidget::buildToolbar(wxSizer* root) {
    auto& tm = ThemeManager::instance();

    auto* bar = new wxPanel(scroll_, wxID_ANY, wxDefaultPosition, wxSize(-1, 72));
    bar->SetBackgroundColour(tm.surface());

    auto* sizer = new wxBoxSizer(wxHORIZONTAL);

    auto makeLabel = [&](const wxString& text) {
        auto* lbl = new wxStaticText(bar, wxID_ANY, text);
        lbl->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        lbl->SetForegroundColour(tm.muted());
        lbl->SetBackgroundColour(tm.surface());
        return lbl;
    };

    auto* periodLabel = makeLabel(utf8::U("Период:"));
    periodChoice_ = new wxChoice(bar, wxID_ANY, wxDefaultPosition, wxSize(200, 36));
    periodChoice_->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    periodChoice_->SetBackgroundColour(tm.surface());
    periodChoice_->SetForegroundColour(tm.text());
    periodChoice_->Append(utf8::U("Последние 7 дней"));
    periodChoice_->Append(utf8::U("Последние 30 дней"));
    periodChoice_->Append(utf8::U("Квартал"));
    periodChoice_->Append(utf8::U("Всё время"));
    periodChoice_->SetSelection(1);
    periodChoice_->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { loadReport(); });

    auto* typeLabel = makeLabel(utf8::U("Тип отчёта:"));
    typeChoice_ = new wxChoice(bar, wxID_ANY, wxDefaultPosition, wxSize(220, 36));
    typeChoice_->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    typeChoice_->SetBackgroundColour(tm.surface());
    typeChoice_->SetForegroundColour(tm.text());
    typeChoice_->Append(utf8::U("Заказы по статусам"));
    typeChoice_->Append(utf8::U("Расход запчастей"));
    typeChoice_->Append(utf8::U("Работа мастеров"));
    typeChoice_->SetSelection(0);
    typeChoice_->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { loadReport(); });

    auto* exportBtn = new wxButton(bar, wxID_ANY, utf8::U("Экспорт в Excel"),
                                   wxDefaultPosition, wxSize(170, 36), wxBORDER_NONE);
    exportBtn->SetBackgroundColour(tm.surface());
    exportBtn->SetForegroundColour(tm.text());
    exportBtn->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    exportBtn->Bind(wxEVT_BUTTON, &ReportsWidget::onExport, this);

    auto* printBtn = new wxButton(bar, wxID_ANY, utf8::U("Печать"),
                                  wxDefaultPosition, wxSize(130, 36), wxBORDER_NONE);
    printBtn->SetBackgroundColour(tm.surface());
    printBtn->SetForegroundColour(tm.text());
    printBtn->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    printBtn->Bind(wxEVT_BUTTON, &ReportsWidget::onPrint, this);

    sizer->Add(periodLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 20);
    sizer->Add(periodChoice_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
    sizer->AddSpacer(24);
    sizer->Add(typeLabel, 0, wxALIGN_CENTER_VERTICAL);
    sizer->Add(typeChoice_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
    sizer->AddStretchSpacer(1);
    sizer->Add(exportBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    sizer->Add(printBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 20);

    bar->SetSizer(sizer);
    root->Add(bar, 0, wxEXPAND | wxLEFT | wxRIGHT, 24);
}

void ReportsWidget::buildStatCards(wxSizer* root) {
    auto& tm = ThemeManager::instance();

    auto* row = new wxBoxSizer(wxHORIZONTAL);

    auto makeCard = [&](const wxString& title, wxStaticText** outValue, wxColour valueColor) {
        auto* card = new wxPanel(scroll_, wxID_ANY, wxDefaultPosition, wxSize(-1, 140));
        card->SetBackgroundColour(tm.surface());

        auto* s = new wxBoxSizer(wxVERTICAL);

        auto* value = new wxStaticText(card, wxID_ANY, "0");
        value->SetFont(wxFont(30, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        value->SetForegroundColour(valueColor);
        value->SetBackgroundColour(tm.surface());

        auto* label = new wxStaticText(card, wxID_ANY, title);
        label->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        label->SetForegroundColour(tm.muted());
        label->SetBackgroundColour(tm.surface());

        s->AddStretchSpacer(1);
        s->Add(value, 0, wxLEFT | wxRIGHT, 24);
        s->AddSpacer(4);
        s->Add(label, 0, wxLEFT | wxRIGHT, 24);
        s->AddStretchSpacer(1);
        card->SetSizer(s);

        *outValue = value;
        return card;
    };

    auto* c1 = makeCard(utf8::U("Всего заказов\nза период"), &totalOrdersValue_, tm.primary());
    auto* c2 = makeCard(utf8::U("Выручка\nстоимость работ"), &revenueValue_, tm.success());
    auto* c3 = makeCard(utf8::U("Среднее время\nот приёма до выдачи"), &avgTimeValue_, tm.warning());

    row->Add(c1, 1, wxEXPAND | wxRIGHT, 12);
    row->Add(c2, 1, wxEXPAND | wxRIGHT, 12);
    row->Add(c3, 1, wxEXPAND);

    root->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT, 24);
}

void ReportsWidget::buildCharts(wxSizer* root) {
    auto& tm = ThemeManager::instance();

    auto* row = new wxBoxSizer(wxHORIZONTAL);

    auto* left = new wxPanel(scroll_, wxID_ANY);
    left->SetBackgroundColour(tm.surface());

    auto* ls = new wxBoxSizer(wxVERTICAL);

    auto* lTitle = new wxStaticText(left, wxID_ANY, utf8::U("Распределение по статусам"));
    lTitle->SetFont(wxFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    lTitle->SetForegroundColour(tm.text());
    lTitle->SetBackgroundColour(tm.surface());
    ls->Add(lTitle, 0, wxALL, 20);

    barChart_ = new BarChartPanel(left);
    ls->Add(barChart_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 20);

    left->SetSizer(ls);

    auto* right = new wxPanel(scroll_, wxID_ANY, wxDefaultPosition, wxSize(340, -1));
    right->SetBackgroundColour(tm.surface());

    auto* rs = new wxBoxSizer(wxVERTICAL);

    auto* rTitle = new wxStaticText(right, wxID_ANY, utf8::U("Доля"));
    rTitle->SetFont(wxFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    rTitle->SetForegroundColour(tm.text());
    rTitle->SetBackgroundColour(tm.surface());
    rs->Add(rTitle, 0, wxALL, 20);

    donutChart_ = new DonutChartPanel(right);
    rs->Add(donutChart_, 0, wxEXPAND | wxLEFT | wxRIGHT, 12);

    auto* legend = new wxPanel(right, wxID_ANY);
    legend->SetBackgroundColour(tm.surface());

    auto* legSizer = new wxBoxSizer(wxVERTICAL);

    auto makeLegendRow = [&](wxColour c, const wxString& name, wxStaticText** outPct) {
        auto* rowSizer = new wxBoxSizer(wxHORIZONTAL);

        auto* swatch = new wxPanel(legend, wxID_ANY, wxDefaultPosition, wxSize(10, 10));
        swatch->SetBackgroundColour(c);

        auto* nameLbl = new wxStaticText(legend, wxID_ANY, name);
        nameLbl->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        nameLbl->SetForegroundColour(tm.text());
        nameLbl->SetBackgroundColour(tm.surface());

        auto* pct = new wxStaticText(legend, wxID_ANY, "0%");
        pct->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        pct->SetForegroundColour(tm.muted());
        pct->SetBackgroundColour(tm.surface());

        rowSizer->Add(swatch, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
        rowSizer->Add(nameLbl, 1, wxALIGN_CENTER_VERTICAL);
        rowSizer->Add(pct, 0, wxALIGN_CENTER_VERTICAL);

        *outPct = pct;
        return rowSizer;
    };

    legSizer->Add(makeLegendRow(colorForStatus("Принят"),
        utf8::U("Принят"), &legendAcceptedPct_), 0, wxEXPAND | wxBOTTOM, 8);
    legSizer->Add(makeLegendRow(colorForStatus("Диагностика"),
        utf8::U("Диагностика"), &legendDiagPct_), 0, wxEXPAND | wxBOTTOM, 8);
    legSizer->Add(makeLegendRow(colorForStatus("В ремонте"),
        utf8::U("В ремонте"), &legendRepairPct_), 0, wxEXPAND | wxBOTTOM, 8);
    legSizer->Add(makeLegendRow(colorForStatus("Ожидает запчасть"),
        utf8::U("Ожидает запчасть"), &legendWaitingPct_), 0, wxEXPAND | wxBOTTOM, 8);
    legSizer->Add(makeLegendRow(colorForStatus("Готов"),
        utf8::U("Готов"), &legendReadyPct_), 0, wxEXPAND | wxBOTTOM, 8);
    legSizer->Add(makeLegendRow(colorForStatus("Выдан"),
        utf8::U("Выдан"), &legendIssuedPct_), 0, wxEXPAND);

    legend->SetSizer(legSizer);
    rs->Add(legend, 0, wxEXPAND | wxALL, 20);

    right->SetSizer(rs);

    row->Add(left, 2, wxEXPAND | wxRIGHT, 12);
    row->Add(right, 1, wxEXPAND);

    root->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT, 24);
}

void ReportsWidget::buildTable(wxSizer* root) {
    auto& tm = ThemeManager::instance();

    auto* card = new wxPanel(scroll_, wxID_ANY);
    card->SetBackgroundColour(tm.surface());

    auto* s = new wxBoxSizer(wxVERTICAL);

    statsTable_ = new StatsTablePanel(card);
    s->Add(statsTable_, 1, wxEXPAND);

    card->SetSizer(s);
    root->Add(card, 0, wxEXPAND | wxLEFT | wxRIGHT, 24);
}

void ReportsWidget::loadReport() {
    auto& tm = ThemeManager::instance();

    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return;

    int periodSel = periodChoice_ ? periodChoice_->GetSelection() : 1;
    std::string since;
    if (periodSel == 0)      since = "-7 days";
    else if (periodSel == 1) since = "-30 days";
    else if (periodSel == 2) since = "-90 days";
    else                     since = "";

    {
        std::string sqlCount = "SELECT COUNT(*), IFNULL(SUM(total_cost),0) FROM orders";
        if (!since.empty()) sqlCount += " WHERE received_at >= datetime('now', '" + since + "')";

        sqlite3_stmt* stmt = nullptr;
        int total = 0;
        double revenue = 0.0;
        if (sqlite3_prepare_v2(db, sqlCount.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                total = sqlite3_column_int(stmt, 0);
                revenue = sqlite3_column_double(stmt, 1);
            }
            sqlite3_finalize(stmt);
        }

        totalOrdersValue_->SetLabel(wxString::Format("%d", total));

        wxString revStr = wxString::Format("%.0f", revenue);
        revStr += " ";
        revStr += utf8::U("руб.");
        revenueValue_->SetLabel(revStr);
    }

    {
        std::string sqlAvg =
            "SELECT AVG(julianday(completed_at) - julianday(received_at)) "
            "FROM orders WHERE completed_at IS NOT NULL";
        sqlite3_stmt* stmt = nullptr;
        double avgDays = 0.0;
        if (sqlite3_prepare_v2(db, sqlAvg.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                avgDays = sqlite3_column_double(stmt, 0);
            }
            sqlite3_finalize(stmt);
        }
        wxString avgStr = wxString::Format("%.1f", avgDays);
        avgStr += " ";
        avgStr += utf8::U("дн.");
        avgTimeValue_->SetLabel(avgStr);
    }

    lastStats_.clear();
    const std::string statuses[] = {
        "Принят", "Диагностика", "В ремонте",
        "Ожидает запчасть", "Готов", "Выдан"
    };
    int grandTotal = 0;

    for (auto& s : statuses) {
        const char* sql = "SELECT COUNT(*) FROM orders WHERE status = ?;";
        sqlite3_stmt* stmt = nullptr;
        int cnt = 0;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, s.c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(stmt) == SQLITE_ROW) cnt = sqlite3_column_int(stmt, 0);
            sqlite3_finalize(stmt);
        }
        StatusStat st;
        st.name = s;
        st.count = cnt;
        st.color = colorForStatus(s);
        lastStats_.push_back(st);
        grandTotal += cnt;
    }
    lastTotal_ = grandTotal;

    if (barChart_)   barChart_->setData(lastStats_);
    if (donutChart_) donutChart_->setData(lastStats_, grandTotal);
    if (statsTable_) statsTable_->setData(lastStats_, grandTotal);

    auto setPct = [&](wxStaticText* pct, int cnt) {
        if (!pct) return;
        double v = grandTotal > 0 ? (cnt * 100.0 / grandTotal) : 0.0;
        pct->SetLabel(wxString::Format("%.0f%%", v));
    };
    setPct(legendAcceptedPct_, lastStats_[0].count);
    setPct(legendDiagPct_,     lastStats_[1].count);
    setPct(legendRepairPct_,   lastStats_[2].count);
    setPct(legendWaitingPct_,  lastStats_[3].count);
    setPct(legendReadyPct_,    lastStats_[4].count);
    setPct(legendIssuedPct_,   lastStats_[5].count);

    Layout();
}

TableData ReportsWidget::buildTableData() {
    TableData td;
    td.title = utf8::toUtf8(utf8::U("Отчёт по заказам"));
    td.headers = {
        utf8::toUtf8(utf8::U("Статус")),
        utf8::toUtf8(utf8::U("Количество")),
        utf8::toUtf8(utf8::U("Доля, %"))
    };

    for (auto& st : lastStats_) {
        int pct = lastTotal_ > 0
            ? static_cast<int>(std::round(st.count * 100.0 / lastTotal_))
            : 0;
        td.rows.push_back({
            st.name,
            std::to_string(st.count),
            std::to_string(pct)
        });
    }
    return td;
}

void ReportsWidget::onExport(wxCommandEvent&) {
    TableData td = buildTableData();
    if (td.rows.empty()) {
        wxMessageBox(utf8::U("Нет данных для экспорта"),
                     utf8::U("Экспорт"), wxOK | wxICON_INFORMATION, this);
        return;
    }

    wxString fileName = wxFileSelector(
        utf8::U("Сохранить отчёт как CSV"), "", "report.csv", "*.csv",
        "CSV files (*.csv)|*.csv",
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
    if (fileName.IsEmpty()) return;

    std::string path = std::string(fileName.ToUTF8().data());
    if (TableExport::exportCsv(td, path)) {
        wxMessageBox(utf8::U("Отчёт сохранён"), utf8::U("Готово"),
                     wxOK | wxICON_INFORMATION, this);
    } else {
        wxMessageBox(utf8::U("Не удалось сохранить файл"), utf8::U("Ошибка"),
                     wxOK | wxICON_ERROR, this);
    }
}

void ReportsWidget::onPrint(wxCommandEvent&) {
    TableData td = buildTableData();
    if (td.rows.empty()) {
        wxMessageBox(utf8::U("Нет данных для печати"),
                     utf8::U("Печать"), wxOK | wxICON_INFORMATION, this);
        return;
    }

    auto& cfg = ConfigManager::instance();
    std::string outDir = std::string(
        wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath().ToUTF8().data());
    std::string path = outDir + "/report_" + TableExport::timestamp() + ".pdf";

    std::string line1 = utf8::toUtf8(utf8::U("Всего заказов: ")) +
                        std::to_string(lastTotal_);

    if (TableExport::printPdf(td, path, cfg.fontPath(), line1, "")) {
        wxLaunchDefaultApplication(path);
    } else {
        wxMessageBox(utf8::U("Не удалось создать PDF"), utf8::U("Ошибка"),
                     wxOK | wxICON_ERROR, this);
    }
}

}