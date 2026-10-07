#include <fstream>
#include <vector>
#include "ui/ReportsWidget.h"
#include "core/DatabaseManager.h"
#include "core/ThemeManager.h"
#include "core/Types.h"
#include "resources/styles.h"
#include "resources/utf8.h"

#include <sqlite3.h>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace remont {

namespace {

std::string dateFrom() {
    return "1970-01-01";
}

}

ReportsWidget::ReportsWidget(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
{
    auto& tm = ThemeManager::instance();
    SetBackgroundColour(tm.background());

    mainSizer_ = new wxBoxSizer(wxVERTICAL);
    mainSizer_->AddSpacer(20);

    buildToolbar(mainSizer_);
    mainSizer_->AddSpacer(12);
    buildTable(mainSizer_);
    buildSummary(mainSizer_);
    mainSizer_->AddSpacer(20);

    SetSizer(mainSizer_);
    loadReport();
}

void ReportsWidget::buildToolbar(wxSizer* root) {
    auto& tm = ThemeManager::instance();

    auto* bar = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 72));
    bar->SetBackgroundColour(tm.surface());

    auto* sizer = new wxBoxSizer(wxHORIZONTAL);

    auto* typeLabel = new wxStaticText(bar, wxID_ANY, utf8::U("Тип отчёта:"));
    typeLabel->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    typeLabel->SetForegroundColour(tm.muted());
    typeLabel->SetBackgroundColour(tm.surface());

    typeChoice_ = new wxChoice(bar, wxID_ANY, wxDefaultPosition, wxSize(280, 40));
    typeChoice_->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    typeChoice_->SetBackgroundColour(tm.surface());
    typeChoice_->SetForegroundColour(tm.text());
    typeChoice_->Append(utf8::U("Заказы по статусам"));
    typeChoice_->Append(utf8::U("Расход запчастей"));
    typeChoice_->Append(utf8::U("Работа мастеров"));
    typeChoice_->SetSelection(0);
    typeChoice_->Bind(wxEVT_CHOICE, &ReportsWidget::onReportTypeChanged, this);

    auto* periodLabel = new wxStaticText(bar, wxID_ANY, utf8::U("Период:"));
    periodLabel->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    periodLabel->SetForegroundColour(tm.muted());
    periodLabel->SetBackgroundColour(tm.surface());

    periodChoice_ = new wxChoice(bar, wxID_ANY, wxDefaultPosition, wxSize(220, 40));
    periodChoice_->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    periodChoice_->SetBackgroundColour(tm.surface());
    periodChoice_->SetForegroundColour(tm.text());
    periodChoice_->Append(utf8::U("Последние 7 дней"));
    periodChoice_->Append(utf8::U("Последние 30 дней"));
    periodChoice_->Append(utf8::U("Квартал"));
    periodChoice_->Append(utf8::U("Текущий месяц"));
    periodChoice_->Append(utf8::U("Всё время"));
    periodChoice_->SetSelection(1);
    periodChoice_->Bind(wxEVT_CHOICE, &ReportsWidget::onPeriodChanged, this);

    auto* exportBtn = new wxButton(bar, wxID_ANY, utf8::U("Экспорт в CSV"),
                                   wxDefaultPosition, wxSize(180, 40), wxBORDER_NONE);
    exportBtn->SetBackgroundColour(tm.surface());
    exportBtn->SetForegroundColour(tm.text());
    exportBtn->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    exportBtn->Bind(wxEVT_BUTTON, &ReportsWidget::onExport, this);

    sizer->Add(typeLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 16);
    sizer->Add(typeChoice_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
    sizer->AddSpacer(24);
    sizer->Add(periodLabel, 0, wxALIGN_CENTER_VERTICAL);
    sizer->Add(periodChoice_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
    sizer->AddStretchSpacer(1);
    sizer->Add(exportBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);

    bar->SetSizer(sizer);
    root->Add(bar, 0, wxEXPAND | wxLEFT | wxRIGHT, 24);
}

void ReportsWidget::buildTable(wxSizer* root) {
    auto& tm = ThemeManager::instance();

    table_ = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 400),
                            wxLC_REPORT | wxBORDER_NONE);
    table_->SetBackgroundColour(tm.surface());
    table_->SetForegroundColour(tm.text());
    table_->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

    root->Add(table_, 1, wxEXPAND | wxLEFT | wxRIGHT, 24);
}

void ReportsWidget::buildSummary(wxSizer* root) {
    auto& tm = ThemeManager::instance();

    auto* footer = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 44));
    footer->SetBackgroundColour(tm.surface());

    summaryLabel_ = new wxStaticText(footer, wxID_ANY, "");
    summaryLabel_->SetForegroundColour(tm.text());
    summaryLabel_->SetBackgroundColour(tm.surface());
    summaryLabel_->SetFont(wxFont(13, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));

    auto* sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(summaryLabel_, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 16);
    footer->SetSizer(sizer);
    root->Add(footer, 0, wxEXPAND | wxLEFT | wxRIGHT, 24);
}

void ReportsWidget::loadReport() {
    if (!table_) return;

    table_->DeleteAllItems();

    while (table_->GetColumnCount() > 0) {
        table_->DeleteColumn(0);
    }

    if (reportType_ == 0) loadStatusReport();
    else if (reportType_ == 1) loadPartsReport();
    else loadMastersReport();
}

void ReportsWidget::loadStatusReport() {
    auto& tm = ThemeManager::instance();

    table_->AppendColumn(utf8::U("СТАТУС"),     wxLIST_FORMAT_LEFT, 280);
    table_->AppendColumn(utf8::U("КОЛИЧЕСТВО"), wxLIST_FORMAT_RIGHT, 160);
    table_->AppendColumn(utf8::U("ДОЛЯ, %"),    wxLIST_FORMAT_RIGHT, 140);

    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return;

    const char* sql = "SELECT status, COUNT(*) FROM orders GROUP BY status;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return;

    struct Row { std::string status; int count; };
    std::vector<Row> rows;
    int total = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Row r;
        const char* s = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        r.status = s ? s : "";
        r.count = sqlite3_column_int(stmt, 1);
        rows.push_back(r);
        total += r.count;
    }
    sqlite3_finalize(stmt);

    long idx = 0;
    for (auto& r : rows) {
        long row = table_->InsertItem(idx, utf8::U(r.status.c_str()));
        table_->SetItem(row, 1, wxString::Format("%d", r.count));

        double pct = total > 0 ? (r.count * 100.0 / total) : 0.0;
        table_->SetItem(row, 2, wxString::Format("%.1f", pct));
        table_->SetItemTextColour(row, tm.text());
        idx++;
    }

    summaryLabel_->SetLabel(utf8::U("Всего заказов: ") + wxString::Format("%d", total));
}

void ReportsWidget::loadPartsReport() {
    auto& tm = ThemeManager::instance();

    table_->AppendColumn(utf8::U("АРТИКУЛ"),       wxLIST_FORMAT_LEFT, 160);
    table_->AppendColumn(utf8::U("НАИМЕНОВАНИЕ"),  wxLIST_FORMAT_LEFT, 320);
    table_->AppendColumn(utf8::U("ИСПОЛЬЗОВАНО"),  wxLIST_FORMAT_RIGHT, 160);
    table_->AppendColumn(utf8::U("СУММА, ₽"),      wxLIST_FORMAT_RIGHT, 160);

    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return;

    const char* sql =
        "SELECT p.article, p.name, SUM(pu.quantity), SUM(pu.quantity * p.price) "
        "FROM part_usage pu JOIN parts p ON pu.part_id = p.id "
        "GROUP BY p.id ORDER BY SUM(pu.quantity) DESC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return;

    long idx = 0;
    double totalSum = 0.0;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* a = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char* n = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        int qty = sqlite3_column_int(stmt, 2);
        double sum = sqlite3_column_double(stmt, 3);

        long row = table_->InsertItem(idx, utf8::U(a ? a : ""));
        table_->SetItem(row, 1, utf8::U(n ? n : ""));
        table_->SetItem(row, 2, wxString::Format("%d", qty));
        table_->SetItem(row, 3, wxString::Format("%.0f", sum));
        table_->SetItemTextColour(row, tm.text());

        totalSum += sum;
        idx++;
    }
    sqlite3_finalize(stmt);

    summaryLabel_->SetLabel(utf8::U("Итого: ") + wxString::Format("%.0f", totalSum) + utf8::U(" ₽"));
}

void ReportsWidget::loadMastersReport() {
    auto& tm = ThemeManager::instance();

    table_->AppendColumn(utf8::U("МАСТЕР"),     wxLIST_FORMAT_LEFT, 260);
    table_->AppendColumn(utf8::U("ВЫПОЛНЕНО"),  wxLIST_FORMAT_RIGHT, 160);
    table_->AppendColumn(utf8::U("ВЫРУЧКА, ₽"), wxLIST_FORMAT_RIGHT, 180);

    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) return;

    const char* sql =
        "SELECT u.login, COUNT(o.id), SUM(o.total_cost) "
        "FROM orders o JOIN users u ON o.user_id = u.id "
        "GROUP BY u.id ORDER BY COUNT(o.id) DESC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return;

    long idx = 0;
    double totalRevenue = 0.0;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* l = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        int count = sqlite3_column_int(stmt, 1);
        double sum = sqlite3_column_double(stmt, 2);

        long row = table_->InsertItem(idx, utf8::U(l ? l : ""));
        table_->SetItem(row, 1, wxString::Format("%d", count));
        table_->SetItem(row, 2, wxString::Format("%.0f", sum));
        table_->SetItemTextColour(row, tm.text());

        totalRevenue += sum;
        idx++;
    }
    sqlite3_finalize(stmt);

    summaryLabel_->SetLabel(utf8::U("Общая выручка: ") +
                            wxString::Format("%.0f", totalRevenue) + utf8::U(" ₽"));
}

void ReportsWidget::onReportTypeChanged(wxCommandEvent&) {
    reportType_ = typeChoice_->GetSelection();
    loadReport();
}

void ReportsWidget::onPeriodChanged(wxCommandEvent&) {
    period_ = periodChoice_->GetSelection();
    loadReport();
}

void ReportsWidget::onExport(wxCommandEvent&) {
    wxString fileName = wxFileSelector(utf8::U("Сохранить отчёт как CSV"),
                                       "", "report.csv", "*.csv",
                                       "CSV files (*.csv)|*.csv",
                                       wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
    if (fileName.IsEmpty()) return;

    std::ofstream f(fileName.ToStdString());
    if (!f.is_open()) {
        wxMessageBox(utf8::U("Не удалось сохранить файл"),
                     utf8::U("Ошибка"), wxOK | wxICON_ERROR, this);
        return;
    }

    long count = table_->GetItemCount();
    int cols = table_->GetColumnCount();

    for (int c = 0; c < cols; ++c) {
        if (c > 0) f << ";";

        wxListItem item;
        item.SetMask(wxLIST_MASK_TEXT);
        table_->GetColumn(c, item);
        f << std::string(item.GetText().ToUTF8().data());
    }
    f << "\n";

    for (long r = 0; r < count; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (c > 0) f << ";";
            f << std::string(table_->GetItemText(r, c).ToUTF8().data());
        }
        f << "\n";
    }
    f.close();

    wxMessageBox(utf8::U("Отчёт сохранён"), utf8::U("Готово"),
                 wxOK | wxICON_INFORMATION, this);
}

}