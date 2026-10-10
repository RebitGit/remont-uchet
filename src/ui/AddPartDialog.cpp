#include "ui/AddPartDialog.h"
#include "repositories/PartRepository.h"
#include "core/ThemeManager.h"
#include "resources/styles.h"
#include "resources/utf8.h"

namespace remont {

AddPartDialog::AddPartDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, utf8::U("Добавить запчасть"),
               wxDefaultPosition, wxSize(700, 520),
               wxDEFAULT_DIALOG_STYLE)
{
    auto& tm = ThemeManager::instance();
    SetBackgroundColour(tm.surface());

    auto* root = new wxBoxSizer(wxVERTICAL);
    root->AddSpacer(24);

    auto* title = new wxStaticText(this, wxID_ANY, utf8::U("Выберите запчасть из склада"));
    title->SetFont(wxFont(15, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    title->SetForegroundColour(tm.text());
    title->SetBackgroundColour(tm.surface());
    root->Add(title, 0, wxLEFT | wxRIGHT, 32);
    root->AddSpacer(16);

    partsTable_ = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 280),
                                 wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_SIMPLE);
    partsTable_->SetBackgroundColour(tm.surface());
    partsTable_->SetForegroundColour(tm.text());
    partsTable_->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

    partsTable_->AppendColumn(utf8::U("АРТИКУЛ"),      wxLIST_FORMAT_LEFT, 120);
    partsTable_->AppendColumn(utf8::U("НАИМЕНОВАНИЕ"), wxLIST_FORMAT_LEFT, 280);
    partsTable_->AppendColumn(utf8::U("ОСТАТОК"),      wxLIST_FORMAT_RIGHT, 100);
    partsTable_->AppendColumn(utf8::U("ЦЕНА"),         wxLIST_FORMAT_RIGHT, 100);

    partsTable_->Bind(wxEVT_LIST_ITEM_SELECTED, &AddPartDialog::onPartSelected, this);

    root->Add(partsTable_, 0, wxEXPAND | wxLEFT | wxRIGHT, 32);
    root->AddSpacer(20);

    auto* qtyRow = new wxBoxSizer(wxHORIZONTAL);
    auto* qtyLabel = new wxStaticText(this, wxID_ANY, utf8::U("Количество:"));
    qtyLabel->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    qtyLabel->SetForegroundColour(tm.text());
    qtyLabel->SetBackgroundColour(tm.surface());

    qtyField_ = new wxTextCtrl(this, wxID_ANY, "1", wxDefaultPosition, wxSize(120, 36), wxBORDER_SIMPLE);
    qtyField_->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    qtyField_->SetBackgroundColour(tm.surface());
    qtyField_->SetForegroundColour(tm.text());

    qtyRow->Add(qtyLabel, 0, wxALIGN_CENTER_VERTICAL);
    qtyRow->AddSpacer(12);
    qtyRow->Add(qtyField_, 0, wxALIGN_CENTER_VERTICAL);
    qtyRow->AddStretchSpacer(1);

    root->Add(qtyRow, 0, wxEXPAND | wxLEFT | wxRIGHT, 32);

    errorPanel_ = new wxPanel(this, wxID_ANY);
    errorPanel_->SetBackgroundColour(wxColour(0xFE, 0xF2, 0xF2));
    auto* errSizer = new wxBoxSizer(wxHORIZONTAL);
    errorLabel_ = new wxStaticText(errorPanel_, wxID_ANY, "");
    errorLabel_->SetForegroundColour(wxColour(0x99, 0x1B, 0x1B));
    errorLabel_->SetBackgroundColour(wxColour(0xFE, 0xF2, 0xF2));
    errorLabel_->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    errSizer->Add(errorLabel_, 1, wxALL, 8);
    errorPanel_->SetSizer(errSizer);
    errorPanel_->Hide();
    root->Add(errorPanel_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 32);

    root->AddStretchSpacer(1);

    auto* btnRow = new wxBoxSizer(wxHORIZONTAL);
    auto* cancelBtn = new wxButton(this, wxID_ANY, utf8::U("Отмена"),
                                   wxDefaultPosition, wxSize(140, 40), wxBORDER_NONE);
    cancelBtn->SetBackgroundColour(tm.surface());
    cancelBtn->SetForegroundColour(tm.text());
    cancelBtn->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    cancelBtn->Bind(wxEVT_BUTTON, &AddPartDialog::onCancel, this);

    auto* okBtn = new wxButton(this, wxID_ANY, utf8::U("Добавить"),
                               wxDefaultPosition, wxSize(140, 40), wxBORDER_NONE);
    okBtn->SetBackgroundColour(tm.primary());
    okBtn->SetForegroundColour(*wxWHITE);
    okBtn->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    okBtn->Bind(wxEVT_BUTTON, &AddPartDialog::onOk, this);

    btnRow->AddStretchSpacer(1);
    btnRow->Add(cancelBtn, 0, wxRIGHT, 12);
    btnRow->Add(okBtn, 0);

    root->Add(btnRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 32);

    SetSizer(root);
    CentreOnScreen();

    loadParts();
}

void AddPartDialog::loadParts() {
    parts_ = PartRepository::instance().getAll();

    auto& tm = ThemeManager::instance();
    partsTable_->DeleteAllItems();

    long idx = 0;
    for (auto& p : parts_) {
        long row = partsTable_->InsertItem(idx, utf8::U(p.article.c_str()));
        partsTable_->SetItem(row, 1, utf8::U(p.name.c_str()));
        partsTable_->SetItem(row, 2, wxString::Format("%d", p.quantity));
        partsTable_->SetItem(row, 3, wxString::Format("%.0f", p.price));

        partsTable_->SetItemData(row, p.id);
        partsTable_->SetItemTextColour(row, tm.text());
        idx++;
    }
}

void AddPartDialog::onPartSelected(wxListEvent& event) {
    int row = event.GetIndex();
    if (row < 0) return;
    selectedPartId_ = static_cast<int>(partsTable_->GetItemData(row));
}

void AddPartDialog::onOk(wxCommandEvent&) {
    if (selectedPartId_ == 0) {
        errorLabel_->SetLabel(utf8::U("Выберите запчасть из списка"));
        errorPanel_->Show();
        Layout();
        return;
    }

    long qty = 0;
    if (!qtyField_->GetValue().ToLong(&qty) || qty <= 0) {
        errorLabel_->SetLabel(utf8::U("Некорректное количество"));
        errorPanel_->Show();
        Layout();
        return;
    }

    Part selected = PartRepository::instance().findById(selectedPartId_);
    if (selected.quantity < qty) {
        errorLabel_->SetLabel(utf8::U("На складе недостаточно запчастей. Доступно: ")
                              + wxString::Format("%d", selected.quantity));
        errorPanel_->Show();
        Layout();
        return;
    }

    quantity_ = static_cast<int>(qty);
    EndModal(wxID_OK);
}

void AddPartDialog::onCancel(wxCommandEvent&) {
    EndModal(wxID_CANCEL);
}

}