#include "ui/DashboardWidget.h"
#include "repositories/OrderRepository.h"
#include "repositories/ClientRepository.h"
#include "repositories/DeviceRepository.h"
#include "core/Types.h"
#include "core/ThemeManager.h"
#include "resources/styles.h"
#include "resources/utf8.h"

namespace remont {

DashboardWidget::DashboardWidget(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
{
    auto& tm = ThemeManager::instance();
    SetBackgroundColour(tm.background());

    auto* root = new wxBoxSizer(wxVERTICAL);
    root->AddSpacer(20);

    buildStats(root);
    root->AddSpacer(20);
    buildToolbar(root);
    root->AddSpacer(8);
    buildTable(root);
    root->AddSpacer(20);

    SetSizer(root);
    loadOrders();
}

void DashboardWidget::reload() {
    DestroyChildren();

    auto& tm = ThemeManager::instance();
    SetBackgroundColour(tm.background());

    auto* root = new wxBoxSizer(wxVERTICAL);
    root->AddSpacer(20);

    buildStats(root);
    root->AddSpacer(20);
    buildToolbar(root);
    root->AddSpacer(8);
    buildTable(root);
    root->AddSpacer(20);

    SetSizer(root);
    Layout();
    loadOrders();
}

void DashboardWidget::buildStats(wxSizer* root) {
    auto& tm = ThemeManager::instance();

    auto all = OrderRepository::instance().getAll();
    int total = static_cast<int>(all.size());
    int inWork = 0, ready = 0, issued = 0;
    for (auto& o : all) {
        if (o.status == OrderStatus::Diagnostics ||
            o.status == OrderStatus::InRepair ||
            o.status == OrderStatus::WaitingPart) inWork++;
        if (o.status == OrderStatus::Ready) ready++;
        if (o.status == OrderStatus::Issued) issued++;
    }

    auto* row = new wxBoxSizer(wxHORIZONTAL);

    struct Stat { const char* label; int value; wxColour color; };
    Stat stats[] = {
        { "Всего заказов",   total,  tm.primary() },
        { "В работе",        inWork, tm.warning() },
        { "Готовы к выдаче", ready,  tm.success() },
        { "Выдано",          issued, tm.muted()   }
    };

    for (auto& s : stats) {
        auto* card = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 130));
        card->SetBackgroundColour(tm.surface());

        auto* cardSizer = new wxBoxSizer(wxVERTICAL);

        auto* value = new wxStaticText(card, wxID_ANY, wxString::Format("%d", s.value));
        value->SetFont(wxFont(tm.fontSizeTitle() + 8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        value->SetForegroundColour(s.color);
        value->SetBackgroundColour(tm.surface());

        auto* label = new wxStaticText(card, wxID_ANY, utf8::U(s.label));
        label->SetForegroundColour(tm.muted());
        label->SetBackgroundColour(tm.surface());
        label->SetFont(wxFont(tm.fontSizeSmall(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

        cardSizer->AddSpacer(16);
        cardSizer->Add(value, 0, wxLEFT | wxRIGHT, 20);
        cardSizer->AddSpacer(6);
        cardSizer->Add(label, 0, wxLEFT | wxRIGHT | wxBOTTOM, 20);

        card->SetSizer(cardSizer);
        row->Add(card, 1, wxEXPAND | wxALL, 8);
    }

    root->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT, 12);
}

void DashboardWidget::buildToolbar(wxSizer* root) {
    auto& tm = ThemeManager::instance();

    auto* bar = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 72));
    bar->SetBackgroundColour(tm.surface());

    auto* sizer = new wxBoxSizer(wxHORIZONTAL);

    search_ = new wxTextCtrl(bar, wxID_ANY, "", wxDefaultPosition, wxSize(300, 36));
    search_->SetHint(utf8::U("Поиск по заказам..."));
    search_->SetFont(wxFont(tm.fontSizeSmall(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    search_->SetBackgroundColour(tm.surface());
    search_->SetForegroundColour(tm.text());

    auto* statusFilter = new wxChoice(bar, wxID_ANY, wxDefaultPosition, wxSize(200, 36));
    statusFilter->SetFont(wxFont(tm.fontSizeSmall(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    statusFilter->SetBackgroundColour(tm.surface());
    statusFilter->SetForegroundColour(tm.text());
    statusFilter->Append(utf8::U("Все"));
    statusFilter->Append(utf8::U("Принят"));
    statusFilter->Append(utf8::U("Диагностика"));
    statusFilter->Append(utf8::U("В ремонте"));
    statusFilter->Append(utf8::U("Ожидает запчасть"));
    statusFilter->Append(utf8::U("Готов"));
    statusFilter->Append(utf8::U("Выдан"));
    statusFilter->SetSelection(0);

    auto* exportBtn = new wxButton(bar, wxID_ANY, utf8::U("Экспорт"),
                                   wxDefaultPosition, wxSize(120, 36),
                                   wxBORDER_NONE);
    exportBtn->SetBackgroundColour(tm.surface());
    exportBtn->SetForegroundColour(tm.text());
    exportBtn->SetFont(wxFont(tm.fontSizeSmall(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

    auto* newOrderBtn = new wxButton(bar, wxID_ANY, utf8::U("+ Новый заказ"),
                                     wxDefaultPosition, wxSize(170, 36),
                                     wxBORDER_NONE);
    newOrderBtn->SetBackgroundColour(tm.primary());
    newOrderBtn->SetForegroundColour(*wxWHITE);
    newOrderBtn->SetFont(wxFont(tm.fontSizeSmall(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));

    newOrderBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        if (onNewOrder_) onNewOrder_();
    });

    sizer->Add(search_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 16);
    sizer->Add(statusFilter, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 12);
    sizer->AddStretchSpacer(1);
    sizer->Add(exportBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    sizer->Add(newOrderBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);

    bar->SetSizer(sizer);
    root->Add(bar, 0, wxEXPAND | wxLEFT | wxRIGHT, 12);
}

void DashboardWidget::buildTable(wxSizer* root) {
    auto& tm = ThemeManager::instance();

    table_ = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 500),
                            wxLC_REPORT | wxBORDER_NONE);
    table_->SetBackgroundColour(tm.surface());
    table_->SetForegroundColour(tm.text());
    table_->SetFont(wxFont(tm.fontSizeSmall(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

    table_->AppendColumn(utf8::U("№"),            wxLIST_FORMAT_LEFT, 100);
    table_->AppendColumn(utf8::U("КЛИЕНТ"),       wxLIST_FORMAT_LEFT, 260);
    table_->AppendColumn(utf8::U("УСТРОЙСТВО"),   wxLIST_FORMAT_LEFT, 280);
    table_->AppendColumn(utf8::U("СТАТУС"),       wxLIST_FORMAT_LEFT, 180);
    table_->AppendColumn(utf8::U("ДАТА ПРИЁМА"),  wxLIST_FORMAT_LEFT, 160);
    table_->AppendColumn(utf8::U("ДЕЙСТВИЯ"),     wxLIST_FORMAT_LEFT, 120);

    root->Add(table_, 1, wxEXPAND | wxLEFT | wxRIGHT, 12);
}

void DashboardWidget::loadOrders() {
    if (!table_) return;

    auto& tm = ThemeManager::instance();
    table_->DeleteAllItems();

    auto orders = OrderRepository::instance().getAll();

    long idx = 0;
    for (auto& o : orders) {
        Client c = ClientRepository::instance().findById(o.clientId);
        Device d = DeviceRepository::instance().findById(o.deviceId);

        long row = table_->InsertItem(idx, utf8::U(o.orderNumber.c_str()));
        table_->SetItem(row, 1, utf8::U(c.fullName.c_str()));
        table_->SetItem(row, 2, utf8::U(d.model.c_str()));
        table_->SetItem(row, 3, utf8::U(statusToString(o.status).c_str()));
        table_->SetItem(row, 4, utf8::U(o.receivedAt.c_str()));
        table_->SetItem(row, 5, utf8::U("👁  🖨"));
        idx++;
    }

    table_->SetTextColour(tm.text());
    table_->SetBackgroundColour(tm.surface());
    table_->Refresh();
}

}