#include "ui/PartFormDialog.h"
#include "resources/styles.h"
#include "resources/utf8.h"

namespace remont {

PartFormDialog::PartFormDialog(wxWindow* parent, const Part& part)
    : wxDialog(parent, wxID_ANY,
               part.id ? utf8::U("Редактировать позицию") : utf8::U("Добавить позицию"),
               wxDefaultPosition, wxSize(720, 480),
               wxDEFAULT_DIALOG_STYLE),
      part_(part),
      isEdit_(part.id != 0)
{
    SetBackgroundColour(styles::Surface);

    auto* root = new wxBoxSizer(wxVERTICAL);
    root->AddSpacer(32);

    auto makeLabel = [&](const wxString& text, bool required) {
        auto* sizer = new wxBoxSizer(wxHORIZONTAL);
        auto* lbl = new wxStaticText(this, wxID_ANY, text);
        lbl->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        lbl->SetForegroundColour(wxColour(0x37, 0x41, 0x51));
        sizer->Add(lbl, 0, wxALIGN_CENTER_VERTICAL);
        if (required) {
            auto* star = new wxStaticText(this, wxID_ANY, " *");
            star->SetForegroundColour(styles::Danger);
            star->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
            sizer->Add(star, 0, wxALIGN_CENTER_VERTICAL);
        }
        return sizer;
    };

    auto makeInput = [&](const wxString& value, const wxString& hint) {
        auto* field = new wxTextCtrl(this, wxID_ANY, value, wxDefaultPosition,
                                     wxSize(-1, 38), wxBORDER_SIMPLE);
        field->SetHint(utf8::U(hint.mb_str()));
        field->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        field->SetBackgroundColour(*wxWHITE);
        return field;
    };

    auto* row1 = new wxBoxSizer(wxHORIZONTAL);

    auto* articleCol = new wxBoxSizer(wxVERTICAL);
    articleCol->Add(makeLabel(utf8::U("Артикул"), true), 0);
    articleCol->AddSpacer(6);
    articleField_ = makeInput(utf8::U(part_.article.c_str()), "A001");
    articleCol->Add(articleField_, 0, wxEXPAND);

    auto* priceCol = new wxBoxSizer(wxVERTICAL);
    priceCol->Add(makeLabel(utf8::U("Цена (₽)"), true), 0);
    priceCol->AddSpacer(6);
    priceField_ = makeInput(
        part_.id ? wxString::Format("%.0f", part_.price) : wxString(""),
        "1000");
    priceCol->Add(priceField_, 0, wxEXPAND);

    row1->Add(articleCol, 1, wxEXPAND);
    row1->AddSpacer(20);
    row1->Add(priceCol, 1, wxEXPAND);

    root->Add(row1, 0, wxEXPAND | wxLEFT | wxRIGHT, 32);
    root->AddSpacer(16);

    auto* nameCol = new wxBoxSizer(wxVERTICAL);
    nameCol->Add(makeLabel(utf8::U("Наименование"), true), 0);
    nameCol->AddSpacer(6);
    nameField_ = makeInput(utf8::U(part_.name.c_str()), "Матрица 15.6 FHD");
    nameCol->Add(nameField_, 0, wxEXPAND);

    root->Add(nameCol, 0, wxEXPAND | wxLEFT | wxRIGHT, 32);
    root->AddSpacer(16);

    auto* row3 = new wxBoxSizer(wxHORIZONTAL);

    auto* qtyCol = new wxBoxSizer(wxVERTICAL);
    qtyCol->Add(makeLabel(utf8::U("Количество"), true), 0);
    qtyCol->AddSpacer(6);
    qtyField_ = makeInput(
        part_.id ? wxString::Format("%d", part_.quantity) : wxString("1"),
        "1");
    qtyCol->Add(qtyField_, 0, wxEXPAND);

    auto* minCol = new wxBoxSizer(wxVERTICAL);
    minCol->Add(makeLabel(utf8::U("Минимальный остаток"), false), 0);
    minCol->AddSpacer(6);
    minQtyField_ = makeInput(
        part_.id ? wxString::Format("%d", part_.minQuantity) : wxString("3"),
        "3");
    minCol->Add(minQtyField_, 0, wxEXPAND);

    row3->Add(qtyCol, 1, wxEXPAND);
    row3->AddSpacer(20);
    row3->Add(minCol, 1, wxEXPAND);

    root->Add(row3, 0, wxEXPAND | wxLEFT | wxRIGHT, 32);

    errorPanel_ = new wxPanel(this, wxID_ANY);
    errorPanel_->SetBackgroundColour(wxColour(0xFE, 0xF2, 0xF2));
    auto* errSizer = new wxBoxSizer(wxHORIZONTAL);
    errorLabel_ = new wxStaticText(errorPanel_, wxID_ANY, "");
    errorLabel_->SetForegroundColour(wxColour(0x99, 0x1B, 0x1B));
    errorLabel_->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    errSizer->Add(errorLabel_, 1, wxALL, 8);
    errorPanel_->SetSizer(errSizer);
    errorPanel_->Hide();
    root->Add(errorPanel_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 32);

    root->AddStretchSpacer(1);

    auto* btnRow = new wxBoxSizer(wxHORIZONTAL);

    auto* cancelBtn = new wxButton(this, wxID_ANY, utf8::U("Отмена"),
                                   wxDefaultPosition, wxSize(130, 42), wxBORDER_NONE);
    cancelBtn->SetBackgroundColour(styles::Surface);
    cancelBtn->SetForegroundColour(styles::Text);
    cancelBtn->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

    auto* saveBtn = new wxButton(this, wxID_ANY,
                                 isEdit_ ? utf8::U("Сохранить") : utf8::U("Добавить"),
                                 wxDefaultPosition, wxSize(130, 42), wxBORDER_NONE);
    saveBtn->SetBackgroundColour(styles::Primary);
    saveBtn->SetForegroundColour(*wxWHITE);
    saveBtn->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));

    btnRow->AddStretchSpacer(1);
    btnRow->Add(cancelBtn, 0, wxRIGHT, 12);
    btnRow->Add(saveBtn, 0);

    root->Add(btnRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 32);

    SetSizer(root);
    Centre();

    cancelBtn->Bind(wxEVT_BUTTON, &PartFormDialog::onCancel, this);
    saveBtn->Bind(wxEVT_BUTTON, &PartFormDialog::onSave, this);
}

void PartFormDialog::onSave(wxCommandEvent&) {
    wxString article = articleField_->GetValue().Trim().Trim(false);
    wxString price   = priceField_->GetValue().Trim().Trim(false);
    wxString name    = nameField_->GetValue().Trim().Trim(false);
    wxString qty     = qtyField_->GetValue().Trim().Trim(false);
    wxString minQty  = minQtyField_->GetValue().Trim().Trim(false);

    if (article.IsEmpty() || price.IsEmpty() || name.IsEmpty() || qty.IsEmpty()) {
        errorLabel_->SetLabel(utf8::U("Заполните все обязательные поля"));
        errorPanel_->Show();
        Layout();
        Fit();
        return;
    }

    long priceVal = 0, qtyVal = 0, minQtyVal = 0;
    if (!price.ToLong(&priceVal) || priceVal < 0) {
        errorLabel_->SetLabel(utf8::U("Некорректная цена"));
        errorPanel_->Show();
        Layout();
        Fit();
        return;
    }
    if (!qty.ToLong(&qtyVal) || qtyVal < 0) {
        errorLabel_->SetLabel(utf8::U("Некорректное количество"));
        errorPanel_->Show();
        Layout();
        Fit();
        return;
    }
    if (!minQty.IsEmpty()) {
        if (!minQty.ToLong(&minQtyVal) || minQtyVal < 0) {
            errorLabel_->SetLabel(utf8::U("Некорректный минимальный остаток"));
            errorPanel_->Show();
            Layout();
            Fit();
            return;
        }
    }

    part_.article = utf8::toUtf8(article);
    part_.name = utf8::toUtf8(name);
    part_.price = static_cast<double>(priceVal);
    part_.quantity = static_cast<int>(qtyVal);
    part_.minQuantity = static_cast<int>(minQtyVal);

    EndModal(wxID_OK);
}

void PartFormDialog::onCancel(wxCommandEvent&) {
    EndModal(wxID_CANCEL);
}

}