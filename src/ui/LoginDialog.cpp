#include "ui/LoginDialog.h"
#include "ui/RoundedLogo.h"
#include "services/AuthService.h"
#include "core/ThemeManager.h"
#include "resources/styles.h"
#include "resources/utf8.h"

namespace remont {

namespace {

wxBitmap loadDarkIcon(const wxString& path, int size, const wxColour& colour) {
    wxImage img;
    if (!img.LoadFile(path, wxBITMAP_TYPE_PNG)) {
        return wxBitmap(size, size);
    }
    img.Rescale(size, size, wxIMAGE_QUALITY_HIGH);
    if (!img.HasAlpha()) img.InitAlpha();
    unsigned char* rgb   = img.GetData();
    unsigned char* alpha = img.GetAlpha();
    int pixels = img.GetWidth() * img.GetHeight();
    for (int i = 0; i < pixels; ++i) {
        if (alpha[i] > 0) {
            rgb[i * 3 + 0] = colour.Red();
            rgb[i * 3 + 1] = colour.Green();
            rgb[i * 3 + 2] = colour.Blue();
        }
    }
    return wxBitmap(img);
}

}

LoginDialog::LoginDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, utf8::U("Ремонт-Учёт — Вход"),
               wxDefaultPosition, wxSize(420, 600),
               wxDEFAULT_DIALOG_STYLE)
{
    auto& tm = ThemeManager::instance();
    SetBackgroundColour(tm.background());

    wxColour iconColour(0x37, 0x41, 0x51);
    eyeOpen_   = loadDarkIcon("resources/icons/eye.png",     18, iconColour);
    eyeClosed_ = loadDarkIcon("resources/icons/eye_off.png", 18, iconColour);

    auto* root = new wxBoxSizer(wxVERTICAL);
    root->AddStretchSpacer(1);

    auto* card = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(360, 500));
    card->SetBackgroundColour(tm.surface());

    auto* cardSizer = new wxBoxSizer(wxVERTICAL);

    auto* logo = new RoundedLogo(card, 56);

    auto* title = new wxStaticText(card, wxID_ANY, utf8::U("Ремонт-Учёт"));
    title->SetFont(wxFont(22, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    title->SetForegroundColour(tm.text());
    title->SetBackgroundColour(tm.surface());

    auto* subtitle = new wxStaticText(card, wxID_ANY, utf8::U("Система учёта заказов"));
    subtitle->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    subtitle->SetForegroundColour(tm.muted());
    subtitle->SetBackgroundColour(tm.surface());

    cardSizer->AddSpacer(28);
    cardSizer->Add(logo, 0, wxALIGN_CENTER);
    cardSizer->AddSpacer(18);
    cardSizer->Add(title, 0, wxALIGN_CENTER);
    cardSizer->AddSpacer(4);
    cardSizer->Add(subtitle, 0, wxALIGN_CENTER);
    cardSizer->AddSpacer(24);

    auto makeLabel = [&](const wxString& text) {
        auto* row = new wxBoxSizer(wxHORIZONTAL);
        auto* txt = new wxStaticText(card, wxID_ANY, text);
        txt->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        txt->SetForegroundColour(tm.text());
        txt->SetBackgroundColour(tm.surface());
        auto* star = new wxStaticText(card, wxID_ANY, " *");
        star->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        star->SetForegroundColour(tm.danger());
        star->SetBackgroundColour(tm.surface());
        row->Add(txt, 0, wxALIGN_CENTER_VERTICAL);
        row->Add(star, 0, wxALIGN_CENTER_VERTICAL);
        return row;
    };

    cardSizer->Add(makeLabel(utf8::U("Логин")), 0, wxLEFT | wxRIGHT, 32);
    cardSizer->AddSpacer(6);

    loginField_ = new wxTextCtrl(card, wxID_ANY, "", wxDefaultPosition,
                                 wxSize(-1, 38), wxBORDER_SIMPLE);
    loginField_->SetHint(utf8::U("Введите логин"));
    loginField_->SetBackgroundColour(tm.surface());
    loginField_->SetForegroundColour(tm.text());
    loginField_->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    cardSizer->Add(loginField_, 0, wxEXPAND | wxLEFT | wxRIGHT, 32);
    cardSizer->AddSpacer(14);

    cardSizer->Add(makeLabel(utf8::U("Пароль")), 0, wxLEFT | wxRIGHT, 32);
    cardSizer->AddSpacer(6);

    auto* passBox = new wxPanel(card, wxID_ANY, wxDefaultPosition, wxSize(-1, 40),
                                wxBORDER_SIMPLE);
    passBox->SetBackgroundColour(tm.surface());

    auto* passSizer = new wxBoxSizer(wxHORIZONTAL);

    passwordField_ = new wxTextCtrl(passBox, wxID_ANY, "", wxDefaultPosition,
                                    wxSize(-1, 32), wxTE_PASSWORD | wxBORDER_NONE);
    passwordField_->SetHint(utf8::U("Введите пароль"));
    passwordField_->SetBackgroundColour(tm.surface());
    passwordField_->SetForegroundColour(tm.text());
    passwordField_->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

    passwordPlain_ = new wxTextCtrl(passBox, wxID_ANY, "", wxDefaultPosition,
                                    wxSize(-1, 32), wxBORDER_NONE);
    passwordPlain_->SetHint(utf8::U("Введите пароль"));
    passwordPlain_->SetBackgroundColour(tm.surface());
    passwordPlain_->SetForegroundColour(tm.text());
    passwordPlain_->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    passwordPlain_->Hide();

    if (eyeClosed_.IsOk()) {
        eyeIcon_ = new wxStaticBitmap(passBox, wxID_ANY, eyeClosed_);
    } else {
        eyeIcon_ = new wxStaticBitmap(passBox, wxID_ANY, wxBitmap(18, 18));
    }
    eyeIcon_->SetCursor(wxCursor(wxCURSOR_HAND));

    passSizer->AddSpacer(8);
    passSizer->Add(passwordField_, 1, wxALIGN_CENTER_VERTICAL);
    passSizer->Add(passwordPlain_, 1, wxALIGN_CENTER_VERTICAL);
    passSizer->AddSpacer(6);
    passSizer->Add(eyeIcon_, 0, wxALIGN_CENTER_VERTICAL);
    passSizer->AddSpacer(8);

    passBox->SetSizer(passSizer);

    cardSizer->Add(passBox, 0, wxEXPAND | wxLEFT | wxRIGHT, 32);
    cardSizer->AddSpacer(10);

    errorPanel_ = new wxPanel(card, wxID_ANY);
    errorPanel_->SetBackgroundColour(wxColour(0xFE, 0xF2, 0xF2));
    auto* errSizer = new wxBoxSizer(wxHORIZONTAL);
    errorLabel_ = new wxStaticText(errorPanel_, wxID_ANY, "");
    errorLabel_->SetForegroundColour(wxColour(0x99, 0x1B, 0x1B));
    errorLabel_->SetBackgroundColour(wxColour(0xFE, 0xF2, 0xF2));
    errorLabel_->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    errSizer->Add(errorLabel_, 1, wxALL, 8);
    errorPanel_->SetSizer(errSizer);
    errorPanel_->Hide();
    cardSizer->Add(errorPanel_, 0, wxEXPAND | wxLEFT | wxRIGHT, 32);
    cardSizer->AddSpacer(8);

    rememberCheck_ = new wxCheckBox(card, wxID_ANY, utf8::U("Запомнить меня"));
    rememberCheck_->SetForegroundColour(tm.text());
    rememberCheck_->SetBackgroundColour(tm.surface());
    rememberCheck_->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    cardSizer->Add(rememberCheck_, 0, wxLEFT | wxRIGHT, 32);
    cardSizer->AddSpacer(14);

    loginButton_ = new wxButton(card, wxID_ANY, utf8::U("Войти"),
                                wxDefaultPosition, wxSize(-1, 40),
                                wxBORDER_NONE);
    loginButton_->SetBackgroundColour(tm.primary());
    loginButton_->SetForegroundColour(*wxWHITE);
    loginButton_->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    cardSizer->Add(loginButton_, 0, wxEXPAND | wxLEFT | wxRIGHT, 32);
    cardSizer->AddSpacer(28);

    card->SetSizer(cardSizer);

    root->Add(card, 0, wxALIGN_CENTER);
    root->AddStretchSpacer(1);

    SetSizer(root);
    CentreOnScreen();

    loginButton_->Bind(wxEVT_BUTTON, &LoginDialog::onLogin, this);
    eyeIcon_->Bind(wxEVT_LEFT_UP, &LoginDialog::onTogglePassword, this);
}

wxString LoginDialog::getLogin() const {
    return loginField_->GetValue();
}

wxString LoginDialog::getPassword() const {
    if (passwordVisible_) return passwordPlain_->GetValue();
    return passwordField_->GetValue();
}

void LoginDialog::showError(const wxString& text) {
    errorLabel_->SetLabel(text);
    errorPanel_->Show();
    Layout();
    Fit();
}

void LoginDialog::hideError() {
    errorPanel_->Hide();
    Layout();
}

void LoginDialog::onTogglePassword(wxMouseEvent&) {
    passwordVisible_ = !passwordVisible_;

    if (passwordVisible_) {
        passwordPlain_->SetValue(passwordField_->GetValue());
        passwordField_->Hide();
        passwordPlain_->Show();
        if (eyeOpen_.IsOk()) eyeIcon_->SetBitmap(eyeOpen_);
    } else {
        passwordField_->SetValue(passwordPlain_->GetValue());
        passwordPlain_->Hide();
        passwordField_->Show();
        if (eyeClosed_.IsOk()) eyeIcon_->SetBitmap(eyeClosed_);
    }

    passwordField_->GetParent()->Layout();
}

void LoginDialog::onLogin(wxCommandEvent&) {
    std::string login    = std::string(getLogin().ToUTF8().data());
    std::string password = std::string(getPassword().ToUTF8().data());

    if (login.empty() || password.empty()) {
        showError(utf8::U("Заполните все поля"));
        return;
    }

    User u;
    if (!AuthService::instance().authenticate(login, password, u)) {
        showError(utf8::U("Неверный логин или пароль"));
        return;
    }

    user_ = u;
    hideError();
    EndModal(wxID_OK);
}

}