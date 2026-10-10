#include "ui/DashboardWidget.h"
#include "repositories/OrderRepository.h"
#include "repositories/ClientRepository.h"
#include "repositories/DeviceRepository.h"
#include "core/Types.h"
#include "core/ThemeManager.h"
#include "core/ConfigManager.h"
#include "resources/styles.h"
#include "resources/utf8.h"

#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>

namespace remont {

DashboardWidget::DashboardWidget(wxWindow* parent, const User& user, int onlyMasterId)
    : wxPanel(parent, wxID_ANY),
      user_(user),
      onlyMasterId_(onlyMasterId)
{
    auto& tm = ThemeManager::instance();
    SetBackgroundColour(tm.background());

    auto* root = new wxBoxSizer(wxVERTICAL);
    root->AddSpacer(20);

    buildStats(root);
    root->AddSpacer(20);
    buildToolbar(root);
    root->AddSpacer(8);
    buildList(root);
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
    buildList(root);
    root->AddSpacer(20);

    SetSizer(root);
    Layout();
    loadOrders();
}

void DashboardWidget::buildStats(wxSizer* root) {
    auto& tm = ThemeManager::instance();

    auto all = OrderRepository::instance().getAll();
    int total = 0, inWork = 0, ready = 0, issued = 0;
    for (auto& o : all) {
        if (onlyMasterId_ > 0 && o.masterId != onlyMasterId_) continue;
        total++;
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
    search_->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { applyFilter(); });

    statusFilter_ = new wxChoice(bar, wxID_ANY, wxDefaultPosition, wxSize(200, 36));
    statusFilter_->SetFont(wxFont(tm.fontSizeSmall(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    statusFilter_->SetBackgroundColour(tm.surface());
    statusFilter_->SetForegroundColour(tm.text());
    statusFilter_->Append(utf8::U("Все"));
    statusFilter_->Append(utf8::U("Принят"));
    statusFilter_->Append(utf8::U("Диагностика"));
    statusFilter_->Append(utf8::U("В ремонте"));
    statusFilter_->Append(utf8::U("Ожидает запчасть"));
    statusFilter_->Append(utf8::U("Готов"));
    statusFilter_->Append(utf8::U("Выдан"));
    statusFilter_->SetSelection(0);
    statusFilter_->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { applyFilter(); });

    auto* exportBtn = new wxButton(bar, wxID_ANY, utf8::U("Экспорт"),
                                   wxDefaultPosition, wxSize(120, 36), wxBORDER_NONE);
    exportBtn->SetBackgroundColour(tm.surface());
    exportBtn->SetForegroundColour(tm.text());
    exportBtn->SetFont(wxFont(tm.fontSizeSmall(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    exportBtn->Bind(wxEVT_BUTTON, &DashboardWidget::onExport, this);

    auto* printBtn = new wxButton(bar, wxID_ANY, utf8::U("Печать"),
                                  wxDefaultPosition, wxSize(120, 36), wxBORDER_NONE);
    printBtn->SetBackgroundColour(tm.surface());
    printBtn->SetForegroundColour(tm.text());
    printBtn->SetFont(wxFont(tm.fontSizeSmall(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    printBtn->Bind(wxEVT_BUTTON, &DashboardWidget::onPrint, this);

    sizer->Add(search_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 16);
    sizer->Add(statusFilter_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 12);
    sizer->AddStretchSpacer(1);
    sizer->Add(exportBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    sizer->Add(printBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    // Кнопка «Новый заказ» только если это не личная вкладка мастера
    // и роль позволяет создавать заказы.
    if (onlyMasterId_ == 0 && canWriteOrders(user_.role)) {
        auto* newOrderBtn = new wxButton(bar, wxID_ANY, utf8::U("+ Новый заказ"),
                                         wxDefaultPosition, wxSize(170, 36), wxBORDER_NONE);
        newOrderBtn->SetBackgroundColour(tm.primary());
        newOrderBtn->SetForegroundColour(*wxWHITE);
        newOrderBtn->SetFont(wxFont(tm.fontSizeSmall(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        newOrderBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            if (onNewOrder_) onNewOrder_();
        });
        sizer->Add(newOrderBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);
    }

    bar->SetSizer(sizer);
    root->Add(bar, 0, wxEXPAND | wxLEFT | wxRIGHT, 12);
}

void DashboardWidget::buildHeader(wxSizer* root, wxWindow* parent) {
    auto& tm = ThemeManager::instance();

    header_ = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 40));
    header_->SetBackgroundColour(wxColour(0xF3, 0xF4, 0xF6));

    auto* sizer = new wxBoxSizer(wxHORIZONTAL);

    auto addCol = [&](const wxString& text, int width) {
        auto* label = new wxStaticText(header_, wxID_ANY, text);
        label->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        label->SetForegroundColour(tm.muted());
        label->SetBackgroundColour(wxColour(0xF3, 0xF4, 0xF6));
        label->SetMinSize(wxSize(width, -1));
        sizer->Add(label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
    };

    addCol(utf8::U("№"),           100);
    addCol(utf8::U("КЛИЕНТ"),      360);
    addCol(utf8::U("УСТРОЙСТВО"),  280);
    addCol(utf8::U("СТАТУС"),      180);
    addCol(utf8::U("ДАТА ПРИЁМА"), 190);
    addCol(utf8::U("ДЕЙСТВИЯ"),    120);

    header_->SetSizer(sizer);
    root->Add(header_, 0, wxEXPAND | wxLEFT | wxRIGHT, 12);
}

void DashboardWidget::buildList(wxSizer* root) {
    auto& tm = ThemeManager::instance();

    auto* wrapper = new wxPanel(this, wxID_ANY);
    wrapper->SetBackgroundColour(tm.surface());

    auto* wrapperSizer = new wxBoxSizer(wxVERTICAL);

    buildHeader(wrapperSizer, wrapper);

    auto* sep = new wxPanel(wrapper, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
    sep->SetBackgroundColour(tm.border());
    wrapperSizer->Add(sep, 0, wxEXPAND);

    list_ = new wxScrolledWindow(wrapper, wxID_ANY, wxDefaultPosition, wxSize(-1, 400),
                                 wxVSCROLL | wxBORDER_NONE);
    list_->SetBackgroundColour(tm.surface());
    list_->SetScrollRate(0, 20);
    wrapperSizer->Add(list_, 1, wxEXPAND);

    wrapper->SetSizer(wrapperSizer);
    root->Add(wrapper, 1, wxEXPAND | wxLEFT | wxRIGHT, 12);
}

wxPanel* DashboardWidget::createRow(wxWindow* parent, const Order& o,
                                    const Client& c, const Device& d)
{
    auto& tm = ThemeManager::instance();

    int orderId = o.id;

    auto bindClick = [this, orderId](wxWindow* w) {
        w->SetCursor(wxCursor(wxCURSOR_HAND));
        w->Bind(wxEVT_LEFT_DCLICK, [this, orderId](wxMouseEvent&) {
            if (onViewOrder_) onViewOrder_(orderId);
        });
        w->Bind(wxEVT_LEFT_UP, [this, orderId](wxMouseEvent&) {
            if (onViewOrder_) onViewOrder_(orderId);
        });
    };

    auto* wrapper = new wxPanel(parent, wxID_ANY);
    wrapper->SetBackgroundColour(tm.surface());

    auto* wrapSizer = new wxBoxSizer(wxVERTICAL);

    auto* row = new wxPanel(wrapper, wxID_ANY, wxDefaultPosition, wxSize(-1, 64));
    row->SetBackgroundColour(tm.surface());

    auto* sizer = new wxBoxSizer(wxHORIZONTAL);

    auto addCell = [&](const wxString& text, int width, int fontSize, bool bold, wxColour color) {
        auto* label = new wxStaticText(row, wxID_ANY, text);
        label->SetFont(wxFont(fontSize, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                              bold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL));
        label->SetForegroundColour(color);
        label->SetBackgroundColour(tm.surface());
        label->SetMinSize(wxSize(width, -1));
        sizer->Add(label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
        bindClick(label);
    };

    addCell(utf8::U(o.orderNumber.c_str()), 100, tm.fontSizeSmall(), true, tm.primary());

    auto* clientCol = new wxPanel(row, wxID_ANY, wxDefaultPosition, wxSize(360, -1));
    clientCol->SetBackgroundColour(tm.surface());
    auto* clientSizer = new wxBoxSizer(wxVERTICAL);

    auto* nameLabel = new wxStaticText(clientCol, wxID_ANY, utf8::U(c.fullName.c_str()));
    nameLabel->SetFont(wxFont(tm.fontSizeSmall(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    nameLabel->SetForegroundColour(tm.text());
    nameLabel->SetBackgroundColour(tm.surface());

    auto* phoneLabel = new wxStaticText(clientCol, wxID_ANY, utf8::U(c.phone.c_str()));
    phoneLabel->SetFont(wxFont(tm.fontSizeSmall() - 2, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    phoneLabel->SetForegroundColour(tm.muted());
    phoneLabel->SetBackgroundColour(tm.surface());

    clientSizer->AddStretchSpacer();
    clientSizer->Add(nameLabel, 0);
    clientSizer->Add(phoneLabel, 0);
    clientSizer->AddStretchSpacer();
    clientCol->SetSizer(clientSizer);

    sizer->Add(clientCol, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);

    bindClick(clientCol);
    bindClick(nameLabel);
    bindClick(phoneLabel);

    addCell(utf8::U(d.model.c_str()),                  280, tm.fontSizeSmall(), false, tm.text());
    addCell(utf8::U(statusToString(o.status).c_str()), 180, tm.fontSizeSmall(), false, tm.text());
    addCell(utf8::U(o.receivedAt.c_str()),             190, tm.fontSizeSmall(), false, tm.muted());

    auto* actionsCol = new wxPanel(row, wxID_ANY, wxDefaultPosition, wxSize(120, -1));
    actionsCol->SetBackgroundColour(tm.surface());

    auto* actSizer = new wxBoxSizer(wxHORIZONTAL);

    auto loadIcon = [](const wxString& path, int size) -> wxBitmap {
        wxImage img;
        if (img.LoadFile(path, wxBITMAP_TYPE_PNG)) {
            img.Rescale(size, size, wxIMAGE_QUALITY_HIGH);
            return wxBitmap(img);
        }
        return wxBitmap(size, size);
    };

    wxBitmap eyeBmp   = loadIcon("resources/icons/eye.png", 20);
    wxBitmap printBmp = loadIcon("resources/icons/printer.png", 20);

    auto* viewIcon = new wxStaticBitmap(actionsCol, wxID_ANY, eyeBmp);
    viewIcon->SetBackgroundColour(tm.surface());
    viewIcon->SetCursor(wxCursor(wxCURSOR_HAND));
    viewIcon->SetToolTip(utf8::U("Открыть заказ"));
    viewIcon->Bind(wxEVT_LEFT_UP, [this, orderId](wxMouseEvent&) {
        if (onViewOrder_) onViewOrder_(orderId);
    });

    auto* printIcon = new wxStaticBitmap(actionsCol, wxID_ANY, printBmp);
    printIcon->SetBackgroundColour(tm.surface());
    printIcon->SetCursor(wxCursor(wxCURSOR_HAND));
    printIcon->SetToolTip(utf8::U("Печать акта"));
    printIcon->Bind(wxEVT_LEFT_UP, [this, orderId](wxMouseEvent&) {
        if (onPrintOrder_) onPrintOrder_(orderId);
    });

    actSizer->Add(viewIcon, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
    actSizer->AddSpacer(16);
    actSizer->Add(printIcon, 0, wxALIGN_CENTER_VERTICAL);

    actionsCol->SetSizer(actSizer);
    sizer->Add(actionsCol, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);

    row->SetSizer(sizer);

    auto* sep = new wxPanel(wrapper, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
    sep->SetBackgroundColour(tm.border());

    wrapSizer->Add(row, 0, wxEXPAND);
    wrapSizer->Add(sep, 0, wxEXPAND);
    wrapper->SetSizer(wrapSizer);

    bindClick(wrapper);
    bindClick(row);
    bindClick(sep);

    return wrapper;
}

void DashboardWidget::loadOrders() {
    applyFilter();
}

void DashboardWidget::applyFilter() {
    if (!list_) return;

    list_->DestroyChildren();

    auto* sizer = new wxBoxSizer(wxVERTICAL);

    wxString query = search_ ? search_->GetValue().Trim().Trim(false) : wxString();
    std::string queryUtf8 = std::string(query.ToUTF8().data());

    int statusSel = statusFilter_ ? statusFilter_->GetSelection() : 0;
    wxString statusChoice = statusFilter_ ? statusFilter_->GetStringSelection() : utf8::U("Все");

    auto orders = OrderRepository::instance().getAll();

    for (auto& o : orders) {
        if (onlyMasterId_ > 0 && o.masterId != onlyMasterId_) continue;

        Client c = ClientRepository::instance().findById(o.clientId);
        Device d = DeviceRepository::instance().findById(o.deviceId);

        if (statusSel > 0) {
            std::string orderStatus = statusToString(o.status);
            if (orderStatus != std::string(statusChoice.ToUTF8().data())) {
                continue;
            }
        }

        if (!queryUtf8.empty()) {
            bool match = false;

            if (c.fullName.find(queryUtf8) != std::string::npos) match = true;
            if (o.orderNumber.find(queryUtf8) != std::string::npos) match = true;
            if (d.model.find(queryUtf8) != std::string::npos) match = true;

            if (!match) continue;
        }

        sizer->Add(createRow(list_, o, c, d), 0, wxEXPAND);
    }

    list_->SetSizer(sizer);
    list_->FitInside();
    list_->Layout();
}

TableData DashboardWidget::buildTableData() {
    TableData td;
    td.title = utf8::toUtf8(onlyMasterId_ > 0
                            ? utf8::U("Мои заказы")
                            : utf8::U("Список заказов"));
    td.headers = {
        utf8::toUtf8(utf8::U("№")),
        utf8::toUtf8(utf8::U("Клиент")),
        utf8::toUtf8(utf8::U("Телефон")),
        utf8::toUtf8(utf8::U("Устройство")),
        utf8::toUtf8(utf8::U("Статус")),
        utf8::toUtf8(utf8::U("Дата приёма")),
        utf8::toUtf8(utf8::U("Стоимость, руб."))
    };

    wxString query = search_ ? search_->GetValue().Trim().Trim(false) : wxString();
    std::string queryUtf8 = std::string(query.ToUTF8().data());
    int statusSel = statusFilter_ ? statusFilter_->GetSelection() : 0;
    wxString statusChoice = statusFilter_ ? statusFilter_->GetStringSelection() : utf8::U("Все");

    auto orders = OrderRepository::instance().getAll();
    for (auto& o : orders) {
        if (onlyMasterId_ > 0 && o.masterId != onlyMasterId_) continue;

        Client c = ClientRepository::instance().findById(o.clientId);
        Device d = DeviceRepository::instance().findById(o.deviceId);

        if (statusSel > 0) {
            if (statusToString(o.status) != std::string(statusChoice.ToUTF8().data()))
                continue;
        }
        if (!queryUtf8.empty()) {
            bool match = c.fullName.find(queryUtf8) != std::string::npos ||
                         o.orderNumber.find(queryUtf8) != std::string::npos ||
                         d.model.find(queryUtf8) != std::string::npos;
            if (!match) continue;
        }

        td.rows.push_back({
            o.orderNumber,
            c.fullName,
            c.phone,
            d.model,
            statusToString(o.status),
            o.receivedAt,
            wxString::Format("%.0f", o.totalCost).ToStdString()
        });
    }
    return td;
}

void DashboardWidget::onExport(wxCommandEvent&) {
    TableData td = buildTableData();

    wxString fileName = wxFileSelector(
        utf8::U("Сохранить список заказов"), "", "orders.csv", "*.csv",
        "CSV files (*.csv)|*.csv",
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
    if (fileName.IsEmpty()) return;

    std::string path = std::string(fileName.ToUTF8().data());
    if (TableExport::exportCsv(td, path)) {
        wxMessageBox(utf8::U("Файл сохранён"), utf8::U("Готово"),
                     wxOK | wxICON_INFORMATION, this);
    } else {
        wxMessageBox(utf8::U("Не удалось сохранить файл"), utf8::U("Ошибка"),
                     wxOK | wxICON_ERROR, this);
    }
}

void DashboardWidget::onPrint(wxCommandEvent&) {
    TableData td = buildTableData();
    if (td.rows.empty()) {
        wxMessageBox(utf8::U("Нет данных для печати"), utf8::U("Печать"),
                     wxOK | wxICON_INFORMATION, this);
        return;
    }

    auto& cfg = ConfigManager::instance();
    std::string outDir = std::string(
        wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath().ToUTF8().data());
    std::string path = outDir + "/orders_" + TableExport::timestamp() + ".pdf";

    std::string line1 = utf8::toUtf8(utf8::U("Всего записей: ")) +
                        std::to_string(td.rows.size());

    if (TableExport::printPdf(td, path, cfg.fontPath(), line1, "")) {
        wxLaunchDefaultApplication(path);
    } else {
        wxMessageBox(utf8::U("Не удалось создать PDF"), utf8::U("Ошибка"),
                     wxOK | wxICON_ERROR, this);
    }
}

}