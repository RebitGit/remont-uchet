#include "ui/OrderViewWidget.h"
#include "ui/AddPartDialog.h"
#include "repositories/OrderRepository.h"
#include "repositories/ClientRepository.h"
#include "repositories/DeviceRepository.h"
#include "repositories/PartRepository.h"
#include "services/OrderService.h"
#include "services/WarehouseService.h"
#include "core/DatabaseManager.h"
#include "core/ThemeManager.h"
#include "core/Types.h"
#include "resources/styles.h"
#include "resources/utf8.h"

#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <algorithm>
#include <sqlite3.h>

namespace remont {

OrderViewWidget::OrderViewWidget(wxWindow* parent, const User& user)
    : wxPanel(parent, wxID_ANY),
      user_(user)
{
    auto& tm = ThemeManager::instance();
    SetBackgroundColour(tm.background());

    auto* outer = new wxBoxSizer(wxVERTICAL);

    auto* scroll = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition,
                                        wxDefaultSize, wxVSCROLL | wxBORDER_NONE);
    scroll->SetBackgroundColour(tm.background());
    scroll->SetScrollRate(0, 16);

    auto* root = new wxBoxSizer(wxVERTICAL);
    root->AddSpacer(16);

    buildBreadcrumb(root, scroll);
    root->AddSpacer(12);
    buildHeader(root, scroll);
    root->AddSpacer(20);

    auto* columns = new wxBoxSizer(wxHORIZONTAL);

    auto* leftCol = new wxBoxSizer(wxVERTICAL);
    leftCol->Add(buildInfoCard(scroll), 0, wxEXPAND);
    leftCol->AddSpacer(20);
    leftCol->Add(buildPartsCard(scroll), 0, wxEXPAND);

    auto* rightCol = new wxBoxSizer(wxVERTICAL);
    rightCol->Add(buildStatusCard(scroll), 0, wxEXPAND);
    rightCol->AddSpacer(20);
    rightCol->Add(buildHistoryCard(scroll), 0, wxEXPAND);

    columns->Add(leftCol, 2, wxEXPAND);
    columns->AddSpacer(20);
    columns->Add(rightCol, 1, wxEXPAND);

    root->Add(columns, 0, wxEXPAND);
    root->AddSpacer(24);

    scroll->SetSizer(root);
    scroll->FitInside();

    outer->Add(scroll, 1, wxEXPAND);
    SetSizer(outer);
}

void OrderViewWidget::setOrderId(int orderId) {
    orderId_ = orderId;
    loadOrder();
}

void OrderViewWidget::reload() {
    loadOrder();
}

wxPanel* OrderViewWidget::buildBreadcrumb(wxSizer* root, wxWindow* parent) {
    auto& tm = ThemeManager::instance();

    auto* row = new wxBoxSizer(wxHORIZONTAL);

    auto* back = new wxStaticText(parent, wxID_ANY, utf8::U("Заказы"));
    back->SetForegroundColour(tm.primary());
    back->SetBackgroundColour(tm.background());
    back->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    back->SetCursor(wxCursor(wxCURSOR_HAND));
    back->Bind(wxEVT_LEFT_UP, &OrderViewWidget::onBack, this);

    auto* sep = new wxStaticText(parent, wxID_ANY, "  /  ");
    sep->SetForegroundColour(tm.muted());
    sep->SetBackgroundColour(tm.background());
    sep->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

    auto* current = new wxStaticText(parent, wxID_ANY, utf8::U("Заказ"));
    current->SetForegroundColour(tm.muted());
    current->SetBackgroundColour(tm.background());
    current->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));

    row->Add(back, 0, wxALIGN_CENTER_VERTICAL);
    row->Add(sep, 0, wxALIGN_CENTER_VERTICAL);
    row->Add(current, 0, wxALIGN_CENTER_VERTICAL);

    root->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT, 24);
    return nullptr;
}

wxPanel* OrderViewWidget::buildHeader(wxSizer* root, wxWindow* parent) {
    auto& tm = ThemeManager::instance();

    auto* row = new wxBoxSizer(wxHORIZONTAL);

    titleLabel_ = new wxStaticText(parent, wxID_ANY, utf8::U("Заказ"));
    titleLabel_->SetFont(wxFont(22, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    titleLabel_->SetForegroundColour(tm.text());
    titleLabel_->SetBackgroundColour(tm.background());

    statusBadge_ = new wxStaticText(parent, wxID_ANY, " ");
    statusBadge_->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    statusBadge_->SetMinSize(wxSize(140, 28));

    auto* printBtn = new wxButton(parent, wxID_ANY, utf8::U("Печать акта"),
                                  wxDefaultPosition, wxSize(160, 38), wxBORDER_NONE);
    printBtn->SetBackgroundColour(tm.surface());
    printBtn->SetForegroundColour(tm.text());
    printBtn->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    printBtn->Bind(wxEVT_BUTTON, &OrderViewWidget::onPrint, this);

    row->Add(titleLabel_, 0, wxALIGN_CENTER_VERTICAL);
    row->AddSpacer(16);
    row->Add(statusBadge_, 0, wxALIGN_CENTER_VERTICAL);
    row->AddStretchSpacer(1);
    row->Add(printBtn, 0, wxALIGN_CENTER_VERTICAL);

    root->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT, 24);
    return nullptr;
}

wxPanel* OrderViewWidget::buildInfoCard(wxWindow* parent) {
    auto& tm = ThemeManager::instance();

    auto* card = new wxPanel(parent, wxID_ANY);
    card->SetBackgroundColour(tm.surface());

    auto* sizer = new wxBoxSizer(wxVERTICAL);

    auto* header = new wxStaticText(card, wxID_ANY, utf8::U("Информация"));
    header->SetFont(wxFont(15, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    header->SetForegroundColour(tm.text());
    header->SetBackgroundColour(tm.surface());
    sizer->Add(header, 0, wxALL, 20);

    auto* sep = new wxPanel(card, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
    sep->SetBackgroundColour(tm.border());
    sizer->Add(sep, 0, wxEXPAND);

    auto* grid = new wxFlexGridSizer(2, 16, 24);
    grid->AddGrowableCol(0, 1);
    grid->AddGrowableCol(1, 1);

    auto makeField = [&](const wxString& label, wxStaticText*& out) {
        auto* col = new wxBoxSizer(wxVERTICAL);
        auto* lbl = new wxStaticText(card, wxID_ANY, label);
        lbl->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        lbl->SetForegroundColour(tm.muted());
        lbl->SetBackgroundColour(tm.surface());
        out = new wxStaticText(card, wxID_ANY, "—");
        out->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        out->SetForegroundColour(tm.text());
        out->SetBackgroundColour(tm.surface());
        col->Add(lbl);
        col->AddSpacer(4);
        col->Add(out);
        return col;
    };

    grid->Add(makeField(utf8::U("Клиент"), clientName_), 1, wxEXPAND);
    grid->Add(makeField(utf8::U("Телефон"), clientPhone_), 1, wxEXPAND);
    grid->Add(makeField(utf8::U("Устройство"), deviceModel_), 1, wxEXPAND);
    grid->Add(makeField(utf8::U("Серийный номер"), deviceSerial_), 1, wxEXPAND);

    auto* descCol = new wxBoxSizer(wxVERTICAL);
    auto* descLbl = new wxStaticText(card, wxID_ANY, utf8::U("Описание неисправности"));
    descLbl->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    descLbl->SetForegroundColour(tm.muted());
    descLbl->SetBackgroundColour(tm.surface());
    description_ = new wxStaticText(card, wxID_ANY, "—");
    description_->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    description_->SetForegroundColour(tm.text());
    description_->SetBackgroundColour(tm.surface());
    description_->Wrap(500);
    descCol->Add(descLbl);
    descCol->AddSpacer(4);
    descCol->Add(description_);

    grid->Add(descCol, 2, wxEXPAND);

    grid->Add(makeField(utf8::U("Дата приёма"), receivedAt_), 1, wxEXPAND);
    grid->Add(makeField(utf8::U("Дата готовности"), completedAt_), 1, wxEXPAND);
    grid->Add(makeField(utf8::U("Мастер"), master_), 1, wxEXPAND);
    grid->Add(makeField(utf8::U("Стоимость работы (руб.)"), totalCost_), 1, wxEXPAND);

    sizer->Add(grid, 0, wxEXPAND | wxALL, 20);

    card->SetSizer(sizer);
    return card;
}

wxPanel* OrderViewWidget::buildPartsCard(wxWindow* parent) {
    auto& tm = ThemeManager::instance();

    auto* card = new wxPanel(parent, wxID_ANY);
    card->SetBackgroundColour(tm.surface());

    auto* sizer = new wxBoxSizer(wxVERTICAL);

    auto* headerRow = new wxBoxSizer(wxHORIZONTAL);
    auto* header = new wxStaticText(card, wxID_ANY, utf8::U("Запчасти и материалы"));
    header->SetFont(wxFont(15, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    header->SetForegroundColour(tm.text());
    header->SetBackgroundColour(tm.surface());

    auto* addBtn = new wxButton(card, wxID_ANY, utf8::U("Добавить"),
                                wxDefaultPosition, wxSize(140, 36), wxBORDER_NONE);
    addBtn->SetBackgroundColour(tm.primary());
    addBtn->SetForegroundColour(*wxWHITE);
    addBtn->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    addBtn->Bind(wxEVT_BUTTON, &OrderViewWidget::onAddPart, this);

    headerRow->Add(header, 0, wxALIGN_CENTER_VERTICAL);
    headerRow->AddStretchSpacer(1);
    headerRow->Add(addBtn, 0, wxALIGN_CENTER_VERTICAL);

    sizer->Add(headerRow, 0, wxEXPAND | wxALL, 20);

    auto* sep = new wxPanel(card, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
    sep->SetBackgroundColour(tm.border());
    sizer->Add(sep, 0, wxEXPAND);

    partsTable_ = new wxListCtrl(card, wxID_ANY, wxDefaultPosition, wxSize(-1, 220),
                                 wxLC_REPORT | wxBORDER_NONE);
    partsTable_->SetBackgroundColour(tm.surface());
    partsTable_->SetForegroundColour(tm.text());
    partsTable_->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

    partsTable_->AppendColumn(utf8::U("АРТИКУЛ"),       wxLIST_FORMAT_LEFT, 120);
    partsTable_->AppendColumn(utf8::U("НАИМЕНОВАНИЕ"),  wxLIST_FORMAT_LEFT, 280);
    partsTable_->AppendColumn(utf8::U("КОЛ-ВО"),        wxLIST_FORMAT_RIGHT, 100);
    partsTable_->AppendColumn(utf8::U("ЦЕНА"),          wxLIST_FORMAT_RIGHT, 120);
    partsTable_->AppendColumn(utf8::U("СУММА"),         wxLIST_FORMAT_RIGHT, 120);

    sizer->Add(partsTable_, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);

    sizer->Add(buildTotalsRow(card), 0, wxEXPAND | wxALL, 20);

    card->SetSizer(sizer);
    return card;
}

wxSizer* OrderViewWidget::buildTotalsRow(wxWindow* parent) {
    auto& tm = ThemeManager::instance();

    auto* box = new wxBoxSizer(wxVERTICAL);

    auto* sep = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
    sep->SetBackgroundColour(tm.border());
    box->Add(sep, 0, wxEXPAND | wxBOTTOM, 12);

    auto makeRow = [&](const wxString& label, wxStaticText*& value, bool big) {
        auto* row = new wxBoxSizer(wxHORIZONTAL);
        auto* lbl = new wxStaticText(parent, wxID_ANY, label);
        lbl->SetFont(wxFont(big ? 13 : 11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        lbl->SetForegroundColour(big ? tm.text() : tm.muted());
        lbl->SetBackgroundColour(tm.surface());

        value = new wxStaticText(parent, wxID_ANY, "0");
        value->SetFont(wxFont(big ? 14 : 12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                              big ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL));
        value->SetForegroundColour(big ? tm.primary() : tm.text());
        value->SetBackgroundColour(tm.surface());

        row->Add(lbl, 1, wxALIGN_CENTER_VERTICAL);
        row->Add(value, 0, wxALIGN_CENTER_VERTICAL);
        return row;
    };

    box->Add(makeRow(utf8::U("Запчасти и материалы:"), partsTotal_, false), 0, wxEXPAND | wxBOTTOM, 6);
    box->Add(makeRow(utf8::U("Стоимость работ:"), workCost_, false), 0, wxEXPAND | wxBOTTOM, 12);

    auto* sep2 = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
    sep2->SetBackgroundColour(tm.border());
    box->Add(sep2, 0, wxEXPAND | wxBOTTOM, 12);

    box->Add(makeRow(utf8::U("Итого:"), grandTotal_, true), 0, wxEXPAND);

    return box;
}

wxPanel* OrderViewWidget::buildStatusCard(wxWindow* parent) {
    auto& tm = ThemeManager::instance();

    auto* card = new wxPanel(parent, wxID_ANY);
    card->SetBackgroundColour(tm.surface());

    auto* sizer = new wxBoxSizer(wxVERTICAL);

    auto* header = new wxStaticText(card, wxID_ANY, utf8::U("Статус заказа"));
    header->SetFont(wxFont(15, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    header->SetForegroundColour(tm.text());
    header->SetBackgroundColour(tm.surface());
    sizer->Add(header, 0, wxALL, 20);

    auto* sep = new wxPanel(card, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
    sep->SetBackgroundColour(tm.border());
    sizer->Add(sep, 0, wxEXPAND);

    auto* newStatusLbl = new wxStaticText(card, wxID_ANY, utf8::U("Новый статус"));
    newStatusLbl->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    newStatusLbl->SetForegroundColour(tm.muted());
    newStatusLbl->SetBackgroundColour(tm.surface());
    sizer->Add(newStatusLbl, 0, wxLEFT | wxRIGHT | wxTOP, 20);
    sizer->AddSpacer(6);

    statusChoice_ = new wxChoice(card, wxID_ANY, wxDefaultPosition, wxSize(-1, 38));
    statusChoice_->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    statusChoice_->SetBackgroundColour(tm.surface());
    statusChoice_->SetForegroundColour(tm.text());
    statusChoice_->Append(utf8::U("Принят"));
    statusChoice_->Append(utf8::U("Диагностика"));
    statusChoice_->Append(utf8::U("В ремонте"));
    statusChoice_->Append(utf8::U("Ожидает запчасть"));
    statusChoice_->Append(utf8::U("Готов"));
    statusChoice_->Append(utf8::U("Выдан"));
    sizer->Add(statusChoice_, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    sizer->AddSpacer(14);

    auto* changeBtn = new wxButton(card, wxID_ANY, utf8::U("Сменить статус"),
                                   wxDefaultPosition, wxSize(-1, 42), wxBORDER_NONE);
    changeBtn->SetBackgroundColour(tm.primary());
    changeBtn->SetForegroundColour(*wxWHITE);
    changeBtn->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    changeBtn->Bind(wxEVT_BUTTON, &OrderViewWidget::onChangeStatus, this);
    sizer->Add(changeBtn, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 20);

    card->SetSizer(sizer);
    return card;
}

wxPanel* OrderViewWidget::buildHistoryCard(wxWindow* parent) {
    auto& tm = ThemeManager::instance();

    auto* card = new wxPanel(parent, wxID_ANY);
    card->SetBackgroundColour(tm.surface());

    auto* sizer = new wxBoxSizer(wxVERTICAL);

    auto* header = new wxStaticText(card, wxID_ANY, utf8::U("История изменений"));
    header->SetFont(wxFont(15, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    header->SetForegroundColour(tm.text());
    header->SetBackgroundColour(tm.surface());
    sizer->Add(header, 0, wxALL, 20);

    auto* sep = new wxPanel(card, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
    sep->SetBackgroundColour(tm.border());
    sizer->Add(sep, 0, wxEXPAND);

    historyPanel_ = new wxPanel(card, wxID_ANY);
    historyPanel_->SetBackgroundColour(tm.surface());
    sizer->Add(historyPanel_, 0, wxEXPAND | wxALL, 20);

    card->SetSizer(sizer);
    return card;
}

void OrderViewWidget::loadOrder() {
    if (orderId_ == 0) return;

    auto& tm = ThemeManager::instance();

    order_ = OrderRepository::instance().findById(orderId_);
    if (order_.id == 0) return;

    Client c = ClientRepository::instance().findById(order_.clientId);
    Device d = DeviceRepository::instance().findById(order_.deviceId);

    titleLabel_->SetLabel(utf8::U("Заказ №") + utf8::U(order_.orderNumber.c_str()));

    std::string statusStr = statusToString(order_.status);
    std::string badgeText = " " + statusStr + " ";
    statusBadge_->SetLabel(utf8::U(badgeText.c_str()));

    wxColour badgeBg = wxColour(0xDB, 0xEA, 0xFE);
    wxColour badgeFg = wxColour(0x1E, 0x40, 0xAF);
    if (order_.status == OrderStatus::Diagnostics) { badgeBg = wxColour(0xED, 0xE9, 0xFE); badgeFg = wxColour(0x5B, 0x21, 0xB6); }
    else if (order_.status == OrderStatus::InRepair) { badgeBg = wxColour(0xFE, 0xF3, 0xC7); badgeFg = wxColour(0x92, 0x40, 0x0E); }
    else if (order_.status == OrderStatus::WaitingPart) { badgeBg = wxColour(0xFE, 0xE2, 0xE2); badgeFg = wxColour(0x99, 0x1B, 0x1B); }
    else if (order_.status == OrderStatus::Ready) { badgeBg = wxColour(0xD1, 0xFA, 0xE5); badgeFg = wxColour(0x06, 0x5F, 0x46); }
    else if (order_.status == OrderStatus::Issued) { badgeBg = wxColour(0xF3, 0xF4, 0xF6); badgeFg = wxColour(0x37, 0x41, 0x51); }

    statusBadge_->SetForegroundColour(badgeFg);
    statusBadge_->SetBackgroundColour(badgeBg);

    clientName_->SetLabel(utf8::U(c.fullName.c_str()));
    clientPhone_->SetLabel(utf8::U(c.phone.c_str()));
    deviceModel_->SetLabel(utf8::U(d.model.c_str()));
    deviceSerial_->SetLabel(utf8::U(d.serialNumber.empty() ? "—" : d.serialNumber.c_str()));
    description_->SetLabel(utf8::U(order_.description.c_str()));
    description_->Wrap(500);
    receivedAt_->SetLabel(utf8::U(order_.receivedAt.c_str()));
    completedAt_->SetLabel(order_.completedAt.empty() ? utf8::U("—") : utf8::U(order_.completedAt.c_str()));

    std::string masterName = "Не назначен";
    if (order_.masterId > 0) {
        sqlite3* mdb = DatabaseManager::instance().handle();
        if (mdb) {
            sqlite3_stmt* mstmt = nullptr;
            if (sqlite3_prepare_v2(mdb, "SELECT login FROM users WHERE id = ?;",
                                   -1, &mstmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(mstmt, 1, order_.masterId);
                if (sqlite3_step(mstmt) == SQLITE_ROW) {
                    const char* l = reinterpret_cast<const char*>(sqlite3_column_text(mstmt, 0));
                    if (l && *l) masterName = l;
                }
                sqlite3_finalize(mstmt);
            }
        }
    }
    master_->SetLabel(utf8::U(masterName.c_str()));

    totalCost_->SetLabel(wxString::Format("%.0f", order_.totalCost));

    statusChoice_->SetSelection(static_cast<int>(order_.status));

    partsTable_->DeleteAllItems();

    long idx = 0;
    double totalParts = 0.0;

    sqlite3* db = DatabaseManager::instance().handle();
    if (db) {
        const char* sql =
            "SELECT p.article, p.name, pu.quantity, p.price "
            "FROM part_usage pu JOIN parts p ON pu.part_id = p.id "
            "WHERE pu.order_id = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, orderId_);
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* a = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                const char* n = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                int qty = sqlite3_column_int(stmt, 2);
                double price = sqlite3_column_double(stmt, 3);

                long row = partsTable_->InsertItem(idx, utf8::U(a ? a : ""));
                partsTable_->SetItem(row, 1, utf8::U(n ? n : ""));
                partsTable_->SetItem(row, 2, wxString::Format("%d", qty));
                partsTable_->SetItem(row, 3, wxString::Format("%.0f", price) + utf8::U(" руб."));
                partsTable_->SetItem(row, 4, wxString::Format("%.0f", qty * price) + utf8::U(" руб."));
                partsTable_->SetItemTextColour(row, tm.text());

                totalParts += qty * price;
                idx++;
            }
            sqlite3_finalize(stmt);
        }
    }

    double workCost = order_.totalCost;
    double grandTotal = totalParts + workCost;

    if (partsTotal_) partsTotal_->SetLabel(wxString::Format("%.0f", totalParts) + utf8::U(" руб."));
    if (workCost_)   workCost_->SetLabel(wxString::Format("%.0f", workCost) + utf8::U(" руб."));
    if (grandTotal_) grandTotal_->SetLabel(wxString::Format("%.0f", grandTotal) + utf8::U(" руб."));

    if (historyPanel_) {
        historyPanel_->DestroyChildren();
        auto* hsizer = new wxBoxSizer(wxVERTICAL);

        auto makeEvent = [&](const wxString& text, const wxString& time) {
            auto* row = new wxBoxSizer(wxHORIZONTAL);

            auto* dot = new wxPanel(historyPanel_, wxID_ANY, wxDefaultPosition, wxSize(10, 10));
            dot->SetBackgroundColour(tm.primary());

            auto* col = new wxBoxSizer(wxVERTICAL);
            auto* t1 = new wxStaticText(historyPanel_, wxID_ANY, text);
            t1->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
            t1->SetForegroundColour(tm.text());
            t1->SetBackgroundColour(tm.surface());
            auto* t2 = new wxStaticText(historyPanel_, wxID_ANY, time);
            t2->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
            t2->SetForegroundColour(tm.muted());
            t2->SetBackgroundColour(tm.surface());
            col->Add(t1);
            col->Add(t2);

            row->Add(dot, 0, wxALIGN_TOP | wxTOP, 4);
            row->AddSpacer(10);
            row->Add(col, 1, wxEXPAND);
            return row;
        };

        bool hasHistory = false;

        sqlite3* db2 = DatabaseManager::instance().handle();
        if (db2) {
            const char* sql =
                "SELECT action, created_at FROM operation_log "
                "WHERE entity_type = 'order' AND entity_id = ? "
                "ORDER BY id DESC;";
            sqlite3_stmt* stmt = nullptr;
            if (sqlite3_prepare_v2(db2, sql, -1, &stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(stmt, 1, orderId_);
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    const char* action = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                    const char* createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));

                    hsizer->Add(makeEvent(utf8::U(action ? action : ""),
                                          utf8::U(createdAt ? createdAt : "")),
                                0, wxEXPAND | wxBOTTOM, 12);
                    hasHistory = true;
                }
                sqlite3_finalize(stmt);
            }
        }

        if (!hasHistory) {
            hsizer->Add(makeEvent(utf8::U("Заказ создан. Статус: Принят"),
                                  utf8::U(order_.receivedAt.c_str())), 0, wxEXPAND);
        }

        historyPanel_->SetSizer(hsizer);
        historyPanel_->Layout();
    }

    Layout();
}

void OrderViewWidget::onBack(wxMouseEvent&) {
    wxCommandEvent evt(wxEVT_BUTTON, 2002);
    GetParent()->GetEventHandler()->ProcessEvent(evt);
}

void OrderViewWidget::onPrint(wxCommandEvent&) {
    if (order_.id == 0) return;

    std::string fileName = "act_" + order_.orderNumber + ".pdf";
    std::replace(fileName.begin(), fileName.end(), '#', '_');

    std::string outputPath = std::string(
        wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath().ToUTF8().data())
        + "/" + fileName;

    if (OrderService::instance().printAcceptanceAct(order_.id, outputPath)) {
        wxString msg = utf8::U("Акт сохранён: ") + utf8::U(outputPath.c_str());
        wxMessageBox(msg, utf8::U("Печать акта"), wxOK | wxICON_INFORMATION, this);
    } else {
        wxMessageBox(utf8::U("Не удалось сформировать PDF-акт"),
                     utf8::U("Ошибка"), wxOK | wxICON_ERROR, this);
    }
}

void OrderViewWidget::onAddPart(wxCommandEvent&) {
    if (order_.id == 0) return;

    AddPartDialog dlg(this);
    if (dlg.ShowModal() != wxID_OK) return;

    int partId = dlg.getSelectedPartId();
    int qty = dlg.getQuantity();

    if (partId == 0 || qty <= 0) return;

    if (!WarehouseService::instance().writeOffPart(partId, qty, order_.id, 0)) {
        wxMessageBox(utf8::U("Не удалось списать запчасть. Проверьте остаток."),
                     utf8::U("Ошибка"), wxOK | wxICON_ERROR, this);
        return;
    }

    loadOrder();
}

void OrderViewWidget::onChangeStatus(wxCommandEvent&) {
    if (order_.id == 0) return;

    int sel = statusChoice_->GetSelection();
    if (sel < 0) return;

    OrderStatus newStatus = static_cast<OrderStatus>(sel);

    if (OrderService::instance().changeStatus(order_.id, newStatus)) {
        loadOrder();
        wxMessageBox(utf8::U("Статус изменён"),
                     utf8::U("Готово"), wxOK | wxICON_INFORMATION, this);
    } else {
        wxMessageBox(utf8::U("Не удалось изменить статус"),
                     utf8::U("Ошибка"), wxOK | wxICON_ERROR, this);
    }
}

}