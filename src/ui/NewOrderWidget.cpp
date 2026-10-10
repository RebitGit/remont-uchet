#include "ui/NewOrderWidget.h"
#include "services/OrderService.h"
#include "core/DatabaseManager.h"
#include "core/ThemeManager.h"
#include "resources/styles.h"
#include "resources/utf8.h"

#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/scrolwin.h>
#include <sqlite3.h>
#include <algorithm>

namespace remont {

NewOrderWidget::NewOrderWidget(wxWindow* parent, const User& user)
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
    root->AddSpacer(20);

    buildBreadcrumb(root, scroll);
    root->AddSpacer(12);
    buildClientCard(root, scroll);
    root->AddSpacer(16);
    buildDeviceCard(root, scroll);
    root->AddSpacer(16);
    buildExtraCard(root, scroll);
    root->AddSpacer(16);
    buildButtons(root, scroll);
    root->AddSpacer(20);

    scroll->SetSizer(root);
    scroll->FitInside();

    outer->Add(scroll, 1, wxEXPAND);
    SetSizer(outer);

    loadMasters();
}

void NewOrderWidget::loadMasters() {
    if (!masterChoice_) return;

    masterChoice_->Clear();
    masterIds_.clear();

    masterChoice_->Append(utf8::U("Не назначен"));
    masterIds_.push_back(0);

    sqlite3* db = DatabaseManager::instance().handle();
    if (!db) {
        masterChoice_->SetSelection(0);
        return;
    }

    const char* sql =
        "SELECT id, login FROM users "
        "WHERE role = 'master' AND is_active = 1 "
        "ORDER BY login;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            const char* login = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            if (login && *login) {
                masterIds_.push_back(id);
                masterChoice_->Append(wxString::FromUTF8(login));
            }
        }
        sqlite3_finalize(stmt);
    }

    masterChoice_->SetSelection(0);
}

void NewOrderWidget::reload() {
    loadMasters();
}

void NewOrderWidget::reset() {
    if (fioField_) fioField_->Clear();
    if (phoneField_) phoneField_->Clear();
    if (emailField_) emailField_->Clear();
    if (deviceTypeChoice_) deviceTypeChoice_->SetSelection(0);
    if (modelField_) modelField_->Clear();
    if (serialField_) serialField_->Clear();
    if (descriptionField_) descriptionField_->Clear();
    if (costField_) costField_->SetValue("0");
    if (masterChoice_) masterChoice_->SetSelection(0);
    if (errorPanel_) errorPanel_->Hide();
    Layout();
}

wxPanel* NewOrderWidget::buildBreadcrumb(wxSizer* root, wxWindow* parent) {
    auto& tm = ThemeManager::instance();

    auto* row = new wxBoxSizer(wxHORIZONTAL);

    auto* ordersLink = new wxStaticText(parent, wxID_ANY, utf8::U("Заказы"));
    ordersLink->SetForegroundColour(tm.primary());
    ordersLink->SetBackgroundColour(tm.background());
    ordersLink->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    ordersLink->SetCursor(wxCursor(wxCURSOR_HAND));

    ordersLink->Bind(wxEVT_LEFT_UP, [this](wxMouseEvent&) {
        wxCommandEvent evt(wxEVT_BUTTON, 2001);
        wxWindow::GetParent()->GetEventHandler()->ProcessEvent(evt);
    });

    auto* sep = new wxStaticText(parent, wxID_ANY, "  /  ");
    sep->SetForegroundColour(tm.muted());
    sep->SetBackgroundColour(tm.background());

    auto* current = new wxStaticText(parent, wxID_ANY, utf8::U("Новый заказ"));
    current->SetForegroundColour(tm.muted());
    current->SetBackgroundColour(tm.background());
    current->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));

    row->Add(ordersLink, 0, wxALIGN_CENTER_VERTICAL);
    row->Add(sep, 0, wxALIGN_CENTER_VERTICAL);
    row->Add(current, 0, wxALIGN_CENTER_VERTICAL);

    root->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT, 32);
    return nullptr;
}

wxPanel* NewOrderWidget::buildClientCard(wxSizer* root, wxWindow* parent) {
    auto& tm = ThemeManager::instance();

    auto* card = new wxPanel(parent, wxID_ANY);
    card->SetBackgroundColour(tm.surface());

    auto* sizer = new wxBoxSizer(wxVERTICAL);

    auto* header = new wxStaticText(card, wxID_ANY, utf8::U("Клиент"));
    header->SetFont(wxFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    header->SetForegroundColour(tm.text());
    header->SetBackgroundColour(tm.surface());
    sizer->Add(header, 0, wxALL, 20);

    auto* sep = new wxPanel(card, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
    sep->SetBackgroundColour(tm.border());
    sizer->Add(sep, 0, wxEXPAND);

    auto makeLabel = [&](wxWindow* p, const wxString& text, bool required) {
        auto* row = new wxBoxSizer(wxHORIZONTAL);
        auto* lbl = new wxStaticText(p, wxID_ANY, text);
        lbl->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        lbl->SetForegroundColour(tm.muted());
        lbl->SetBackgroundColour(tm.surface());
        row->Add(lbl, 0, wxALIGN_CENTER_VERTICAL);
        if (required) {
            auto* star = new wxStaticText(p, wxID_ANY, " *");
            star->SetForegroundColour(tm.danger());
            star->SetBackgroundColour(tm.surface());
            star->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
            row->Add(star, 0, wxALIGN_CENTER_VERTICAL);
        }
        return row;
    };

    auto makeInput = [&](wxWindow* p) {
        auto* field = new wxTextCtrl(p, wxID_ANY, "", wxDefaultPosition,
                                     wxSize(-1, 36), wxBORDER_SIMPLE);
        field->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        field->SetBackgroundColour(tm.surface());
        field->SetForegroundColour(tm.text());
        return field;
    };

    auto* fioSizer = new wxBoxSizer(wxVERTICAL);
    fioSizer->Add(makeLabel(card, utf8::U("ФИО клиента"), true), 0);
    fioSizer->AddSpacer(6);
    fioField_ = makeInput(card);
    fioField_->SetHint(utf8::U("Иванов Иван Иванович"));
    fioSizer->Add(fioField_, 0, wxEXPAND);
    sizer->Add(fioSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 20);

    auto* row = new wxBoxSizer(wxHORIZONTAL);

    auto* phoneSizer = new wxBoxSizer(wxVERTICAL);
    phoneSizer->Add(makeLabel(card, utf8::U("Телефон"), true), 0);
    phoneSizer->AddSpacer(6);
    phoneField_ = makeInput(card);
    phoneField_->SetHint(utf8::U("+7 (000) 000-00-00"));
    phoneSizer->Add(phoneField_, 0, wxEXPAND);

    auto* emailSizer = new wxBoxSizer(wxVERTICAL);
    emailSizer->Add(makeLabel(card, utf8::U("Email"), false), 0);
    emailSizer->AddSpacer(6);
    emailField_ = makeInput(card);
    emailField_->SetHint(utf8::U("example@mail.ru"));
    emailSizer->Add(emailField_, 0, wxEXPAND);

    row->Add(phoneSizer, 1, wxEXPAND);
    row->AddSpacer(16);
    row->Add(emailSizer, 1, wxEXPAND);
    sizer->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 20);

    card->SetSizer(sizer);
    root->Add(card, 0, wxEXPAND | wxLEFT | wxRIGHT, 32);
    return card;
}

wxPanel* NewOrderWidget::buildDeviceCard(wxSizer* root, wxWindow* parent) {
    auto& tm = ThemeManager::instance();

    auto* card = new wxPanel(parent, wxID_ANY);
    card->SetBackgroundColour(tm.surface());

    auto* sizer = new wxBoxSizer(wxVERTICAL);

    auto* header = new wxStaticText(card, wxID_ANY, utf8::U("Устройство"));
    header->SetFont(wxFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    header->SetForegroundColour(tm.text());
    header->SetBackgroundColour(tm.surface());
    sizer->Add(header, 0, wxALL, 20);

    auto* sep = new wxPanel(card, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
    sep->SetBackgroundColour(tm.border());
    sizer->Add(sep, 0, wxEXPAND);

    auto makeLabel = [&](const wxString& text, bool required) {
        auto* row = new wxBoxSizer(wxHORIZONTAL);
        auto* lbl = new wxStaticText(card, wxID_ANY, text);
        lbl->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        lbl->SetForegroundColour(tm.muted());
        lbl->SetBackgroundColour(tm.surface());
        row->Add(lbl, 0, wxALIGN_CENTER_VERTICAL);
        if (required) {
            auto* star = new wxStaticText(card, wxID_ANY, " *");
            star->SetForegroundColour(tm.danger());
            star->SetBackgroundColour(tm.surface());
            star->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
            row->Add(star, 0, wxALIGN_CENTER_VERTICAL);
        }
        return row;
    };

    auto* row1 = new wxBoxSizer(wxHORIZONTAL);

    auto* typeSizer = new wxBoxSizer(wxVERTICAL);
    typeSizer->Add(makeLabel(utf8::U("Тип устройства"), true), 0);
    typeSizer->AddSpacer(6);
    deviceTypeChoice_ = new wxChoice(card, wxID_ANY, wxDefaultPosition, wxSize(-1, 36));
    deviceTypeChoice_->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    deviceTypeChoice_->SetBackgroundColour(tm.surface());
    deviceTypeChoice_->SetForegroundColour(tm.text());
    deviceTypeChoice_->Append(utf8::U("Выберите тип..."));
    deviceTypeChoice_->Append(utf8::U("Ноутбук"));
    deviceTypeChoice_->Append(utf8::U("Смартфон"));
    deviceTypeChoice_->Append(utf8::U("Планшет"));
    deviceTypeChoice_->Append(utf8::U("Настольный ПК"));
    deviceTypeChoice_->Append(utf8::U("Монитор"));
    deviceTypeChoice_->Append(utf8::U("Принтер"));
    deviceTypeChoice_->Append(utf8::U("Другое"));
    deviceTypeChoice_->SetSelection(0);
    typeSizer->Add(deviceTypeChoice_, 0, wxEXPAND);

    auto* modelSizer = new wxBoxSizer(wxVERTICAL);
    modelSizer->Add(makeLabel(utf8::U("Модель"), true), 0);
    modelSizer->AddSpacer(6);
    modelField_ = new wxTextCtrl(card, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 36), wxBORDER_SIMPLE);
    modelField_->SetHint(utf8::U("HP Pavilion 15-eh1xxx"));
    modelField_->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    modelField_->SetBackgroundColour(tm.surface());
    modelField_->SetForegroundColour(tm.text());
    modelSizer->Add(modelField_, 0, wxEXPAND);

    row1->Add(typeSizer, 1, wxEXPAND);
    row1->AddSpacer(16);
    row1->Add(modelSizer, 1, wxEXPAND);
    sizer->Add(row1, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 20);

    auto* serialSizer = new wxBoxSizer(wxVERTICAL);
    serialSizer->Add(makeLabel(utf8::U("Серийный номер"), false), 0);
    serialSizer->AddSpacer(6);
    serialField_ = new wxTextCtrl(card, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 36), wxBORDER_SIMPLE);
    serialField_->SetHint(utf8::U("5CD1234567"));
    serialField_->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    serialField_->SetBackgroundColour(tm.surface());
    serialField_->SetForegroundColour(tm.text());
    serialSizer->Add(serialField_, 0, wxEXPAND);
    sizer->Add(serialSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 20);

    auto* descLabel = new wxStaticText(card, wxID_ANY, utf8::U("Описание неисправности"));
    descLabel->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    descLabel->SetForegroundColour(tm.muted());
    descLabel->SetBackgroundColour(tm.surface());
    sizer->Add(descLabel, 0, wxLEFT | wxRIGHT | wxTOP, 20);
    sizer->AddSpacer(6);

    descriptionField_ = new wxTextCtrl(card, wxID_ANY, "", wxDefaultPosition,
                                       wxSize(-1, 100), wxTE_MULTILINE | wxBORDER_SIMPLE);
    descriptionField_->SetHint(utf8::U("Опишите проблему или неисправность устройства..."));
    descriptionField_->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    descriptionField_->SetBackgroundColour(tm.surface());
    descriptionField_->SetForegroundColour(tm.text());
    sizer->Add(descriptionField_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 20);

    card->SetSizer(sizer);
    root->Add(card, 0, wxEXPAND | wxLEFT | wxRIGHT, 32);
    return card;
}

wxPanel* NewOrderWidget::buildExtraCard(wxSizer* root, wxWindow* parent) {
    auto& tm = ThemeManager::instance();

    auto* card = new wxPanel(parent, wxID_ANY);
    card->SetBackgroundColour(tm.surface());

    auto* sizer = new wxBoxSizer(wxVERTICAL);

    auto* header = new wxStaticText(card, wxID_ANY, utf8::U("Дополнительно"));
    header->SetFont(wxFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    header->SetForegroundColour(tm.text());
    header->SetBackgroundColour(tm.surface());
    sizer->Add(header, 0, wxALL, 20);

    auto* sep = new wxPanel(card, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
    sep->SetBackgroundColour(tm.border());
    sizer->Add(sep, 0, wxEXPAND);

    auto* row = new wxBoxSizer(wxHORIZONTAL);

    auto* costSizer = new wxBoxSizer(wxVERTICAL);
    auto* costLabel = new wxStaticText(card, wxID_ANY, utf8::U("Предварительная стоимость (руб.)"));
    costLabel->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    costLabel->SetForegroundColour(tm.muted());
    costLabel->SetBackgroundColour(tm.surface());
    costSizer->Add(costLabel, 0);
    costSizer->AddSpacer(6);
    costField_ = new wxTextCtrl(card, wxID_ANY, "0", wxDefaultPosition, wxSize(-1, 36), wxBORDER_SIMPLE);
    costField_->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    costField_->SetBackgroundColour(tm.surface());
    costField_->SetForegroundColour(tm.text());
    costSizer->Add(costField_, 0, wxEXPAND);

    auto* masterSizer = new wxBoxSizer(wxVERTICAL);
    auto* masterLabel = new wxStaticText(card, wxID_ANY, utf8::U("Мастер"));
    masterLabel->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    masterLabel->SetForegroundColour(tm.muted());
    masterLabel->SetBackgroundColour(tm.surface());
    masterSizer->Add(masterLabel, 0);
    masterSizer->AddSpacer(6);
    masterChoice_ = new wxChoice(card, wxID_ANY, wxDefaultPosition, wxSize(-1, 36));
    masterChoice_->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    masterChoice_->SetBackgroundColour(tm.surface());
    masterChoice_->SetForegroundColour(tm.text());
    masterChoice_->Append(utf8::U("Не назначен"));
    masterChoice_->SetSelection(0);
    masterSizer->Add(masterChoice_, 0, wxEXPAND);

    row->Add(costSizer, 1, wxEXPAND);
    row->AddSpacer(16);
    row->Add(masterSizer, 1, wxEXPAND);
    sizer->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 20);

    card->SetSizer(sizer);
    root->Add(card, 0, wxEXPAND | wxLEFT | wxRIGHT, 32);
    return card;
}

void NewOrderWidget::buildButtons(wxSizer* root, wxWindow* parent) {
    auto& tm = ThemeManager::instance();

    errorPanel_ = new wxPanel(parent, wxID_ANY);
    errorPanel_->SetBackgroundColour(wxColour(0xFE, 0xF2, 0xF2));

    auto* errSizer = new wxBoxSizer(wxHORIZONTAL);
    errorLabel_ = new wxStaticText(errorPanel_, wxID_ANY, "");
    errorLabel_->SetForegroundColour(wxColour(0x99, 0x1B, 0x1B));
    errorLabel_->SetBackgroundColour(wxColour(0xFE, 0xF2, 0xF2));
    errorLabel_->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    errSizer->Add(errorLabel_, 1, wxALL, 10);
    errorPanel_->SetSizer(errSizer);
    errorPanel_->Hide();
    root->Add(errorPanel_, 0, wxEXPAND | wxLEFT | wxRIGHT, 32);

    auto* btnRow = new wxBoxSizer(wxHORIZONTAL);

    auto* cancelBtn = new wxButton(parent, wxID_ANY, utf8::U("Отмена"),
                                   wxDefaultPosition, wxSize(140, 40), wxBORDER_NONE);
    cancelBtn->SetBackgroundColour(tm.surface());
    cancelBtn->SetForegroundColour(tm.text());
    cancelBtn->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    cancelBtn->Bind(wxEVT_BUTTON, &NewOrderWidget::onCancel, this);

    auto* saveBtn = new wxButton(parent, wxID_ANY, utf8::U("Сохранить"),
                                 wxDefaultPosition, wxSize(140, 40), wxBORDER_NONE);
    saveBtn->SetBackgroundColour(tm.surface());
    saveBtn->SetForegroundColour(tm.text());
    saveBtn->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    saveBtn->Bind(wxEVT_BUTTON, &NewOrderWidget::onSave, this);

    auto* savePrintBtn = new wxButton(parent, wxID_ANY, utf8::U("Сохранить и напечатать акт"),
                                      wxDefaultPosition, wxSize(240, 40), wxBORDER_NONE);
    savePrintBtn->SetBackgroundColour(tm.primary());
    savePrintBtn->SetForegroundColour(*wxWHITE);
    savePrintBtn->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    savePrintBtn->Bind(wxEVT_BUTTON, &NewOrderWidget::onSaveAndPrint, this);

    btnRow->AddStretchSpacer(1);
    btnRow->Add(cancelBtn, 0, wxRIGHT, 12);
    btnRow->Add(saveBtn, 0, wxRIGHT, 12);
    btnRow->Add(savePrintBtn, 0);

    root->Add(btnRow, 0, wxEXPAND | wxLEFT | wxRIGHT, 32);
}

bool NewOrderWidget::validate() {
    if (fioField_->GetValue().Trim().Trim(false).IsEmpty() ||
        phoneField_->GetValue().Trim().Trim(false).IsEmpty() ||
        deviceTypeChoice_->GetSelection() == 0 ||
        modelField_->GetValue().Trim().Trim(false).IsEmpty()) {
        errorLabel_->SetLabel(utf8::U("Заполните обязательные поля"));
        errorPanel_->Show();
        Layout();
        return false;
    }
    errorPanel_->Hide();
    Layout();
    return true;
}

bool NewOrderWidget::saveOrder(bool printAfter) {
    if (!validate()) return false;

    std::string clientFullName = std::string(fioField_->GetValue().ToUTF8().data());
    std::string clientPhone = std::string(phoneField_->GetValue().ToUTF8().data());
    std::string clientEmail = std::string(emailField_->GetValue().ToUTF8().data());

    std::string deviceType = std::string(deviceTypeChoice_->GetStringSelection().ToUTF8().data());
    std::string deviceModel = std::string(modelField_->GetValue().ToUTF8().data());
    std::string deviceSerial = std::string(serialField_->GetValue().ToUTF8().data());
    std::string description = std::string(descriptionField_->GetValue().ToUTF8().data());

    long costVal = 0;
    costField_->GetValue().ToLong(&costVal);
    double cost = static_cast<double>(costVal);

    int masterId = 0;
    if (masterChoice_) {
        int sel = masterChoice_->GetSelection();
        if (sel > 0 && sel < static_cast<int>(masterIds_.size())) {
            masterId = masterIds_[sel];
        }
    }

    Order order;
    if (!OrderService::instance().createOrder(
            clientFullName, clientPhone, clientEmail,
            deviceType, deviceModel, deviceSerial,
            description, cost, user_.id, order, masterId)) {
        errorLabel_->SetLabel(utf8::U("Не удалось создать заказ"));
        errorPanel_->Show();
        Layout();
        return false;
    }

    if (printAfter) {
        std::string fileName = "act_" + order.orderNumber + ".pdf";
        std::replace(fileName.begin(), fileName.end(), '#', '_');

        std::string outputPath = std::string(
            wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath().ToUTF8().data())
            + "/" + fileName;

        if (OrderService::instance().printAcceptanceAct(order.id, outputPath)) {
            wxString msg = utf8::U("Акт сохранён: ") + utf8::U(outputPath.c_str());
            wxMessageBox(msg, utf8::U("Печать акта"), wxOK | wxICON_INFORMATION, this);
        } else {
            wxMessageBox(utf8::U("Не удалось сформировать PDF-акт"),
                         utf8::U("Ошибка"), wxOK | wxICON_ERROR, this);
        }
    }

    return true;
}

void NewOrderWidget::onCancel(wxCommandEvent&) {
    reset();
    wxCommandEvent evt(wxEVT_BUTTON, 2001);
    wxWindow::GetParent()->GetEventHandler()->ProcessEvent(evt);
}

void NewOrderWidget::onSave(wxCommandEvent&) {
    if (saveOrder(false)) {
        reset();
        wxCommandEvent evt(wxEVT_BUTTON, 2001);
        wxWindow::GetParent()->GetEventHandler()->ProcessEvent(evt);
    }
}

void NewOrderWidget::onSaveAndPrint(wxCommandEvent&) {
    if (saveOrder(true)) {
        reset();
        wxCommandEvent evt(wxEVT_BUTTON, 2001);
        wxWindow::GetParent()->GetEventHandler()->ProcessEvent(evt);
    }
}

}