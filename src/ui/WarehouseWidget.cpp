#include "ui/WarehouseWidget.h"
#include "ui/PartFormDialog.h"
#include "repositories/PartRepository.h"
#include "services/WarehouseService.h"
#include "core/ThemeManager.h"
#include "core/ConfigManager.h"
#include "core/Types.h"
#include "core/Logger.h"
#include "resources/styles.h"
#include "resources/utf8.h"

#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>

#include <algorithm>
#include <cctype>

namespace remont {

namespace {

std::string toLower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

}

WarehouseWidget::WarehouseWidget(wxWindow* parent, const User& user)
    : wxPanel(parent, wxID_ANY),
      user_(user)
{
    auto& tm = ThemeManager::instance();
    SetBackgroundColour(tm.background());

    bmpEdit_     = loadIcon("resources/icons/action_edit.png", 22);
    bmpWriteOff_ = loadIcon("resources/icons/action_writeoff.png", 22);
    bmpDelete_   = loadIcon("resources/icons/action_delete.png", 22);

    mainSizer_ = new wxBoxSizer(wxVERTICAL);
    mainSizer_->AddSpacer(16);

    buildAlert(mainSizer_);
    mainSizer_->AddSpacer(16);

    buildToolbar(mainSizer_);
    mainSizer_->AddSpacer(8);
    buildTable(mainSizer_);
    buildFooter(mainSizer_);
    mainSizer_->AddSpacer(16);

    SetSizer(mainSizer_);
    loadParts();
}

wxBitmap WarehouseWidget::loadIcon(const wxString& path, int size) {
    wxImage img;
    if (img.LoadFile(path, wxBITMAP_TYPE_PNG)) {
        img.Rescale(size, size, wxIMAGE_QUALITY_HIGH);
        return wxBitmap(img);
    }
    return wxBitmap(size, size);
}

void WarehouseWidget::buildAlert(wxSizer* root) {
    auto& tm = ThemeManager::instance();

    alertPanel_ = new wxPanel(this, wxID_ANY);
    alertPanel_->SetBackgroundColour(wxColour(0xFF, 0xFB, 0xEB));

    auto* sizer = new wxBoxSizer(wxHORIZONTAL);

    alertText_ = new wxStaticText(alertPanel_, wxID_ANY, "");
    alertText_->SetForegroundColour(wxColour(0x92, 0x40, 0x0E));
    alertText_->SetBackgroundColour(wxColour(0xFF, 0xFB, 0xEB));
    alertText_->SetFont(wxFont(tm.fontSizeSmall(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

    sizer->Add(alertText_, 1, wxALL, 14);
    alertPanel_->SetSizer(sizer);

    root->Add(alertPanel_, 0, wxEXPAND | wxLEFT | wxRIGHT, 24);
}

void WarehouseWidget::refreshAlert() {
    if (!alertPanel_ || !alertText_) return;

    auto low = PartRepository::instance().getLowStock();

    if (low.empty()) {
        alertPanel_->Hide();
        Layout();
        return;
    }

    wxString text = utf8::U("Низкий остаток: ") +
                    wxString::Format("%zu ", low.size()) +
                    utf8::U("позиции требуют пополнения — ");
    for (size_t i = 0; i < low.size(); ++i) {
        if (i > 0) text += ", ";
        text += utf8::U(low[i].name.c_str());
    }
    alertText_->SetLabel(text);
    alertPanel_->Show();
    alertPanel_->Layout();
    Layout();
}

void WarehouseWidget::buildToolbar(wxSizer* root) {
    auto& tm = ThemeManager::instance();

    auto* bar = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 72));
    bar->SetBackgroundColour(tm.surface());

    auto* sizer = new wxBoxSizer(wxHORIZONTAL);

    search_ = new wxTextCtrl(bar, wxID_ANY, "", wxDefaultPosition, wxSize(360, 40));
    search_->SetHint(utf8::U("Поиск по наименованию или артикулу..."));
    search_->SetFont(wxFont(tm.fontSizeSmall() + 2, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    search_->SetBackgroundColour(tm.surface());
    search_->SetForegroundColour(tm.text());
    search_->Bind(wxEVT_TEXT, &WarehouseWidget::onSearchChanged, this);

    auto* exportBtn = new wxButton(bar, wxID_ANY, utf8::U("Экспорт"),
                                   wxDefaultPosition, wxSize(150, 40), wxBORDER_NONE);
    exportBtn->SetBackgroundColour(tm.surface());
    exportBtn->SetForegroundColour(tm.text());
    exportBtn->SetFont(wxFont(tm.fontSizeSmall() + 1, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    exportBtn->Bind(wxEVT_BUTTON, &WarehouseWidget::onExport, this);

    auto* printBtn = new wxButton(bar, wxID_ANY, utf8::U("Печать"),
                                  wxDefaultPosition, wxSize(140, 40), wxBORDER_NONE);
    printBtn->SetBackgroundColour(tm.surface());
    printBtn->SetForegroundColour(tm.text());
    printBtn->SetFont(wxFont(tm.fontSizeSmall() + 1, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    printBtn->Bind(wxEVT_BUTTON, &WarehouseWidget::onPrint, this);

    sizer->Add(search_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 16);
    sizer->AddStretchSpacer(1);
    sizer->Add(exportBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    sizer->Add(printBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    if (canWriteWarehouse(user_.role)) {
        auto* addBtn = new wxButton(bar, wxID_ANY, utf8::U("+ Добавить позицию"),
                                    wxDefaultPosition, wxSize(220, 40), wxBORDER_NONE);
        addBtn->SetBackgroundColour(tm.primary());
        addBtn->SetForegroundColour(*wxWHITE);
        addBtn->SetFont(wxFont(tm.fontSizeSmall() + 1, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        addBtn->Bind(wxEVT_BUTTON, &WarehouseWidget::onAddPart, this);
        sizer->Add(addBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);
    }

    bar->SetSizer(sizer);
    root->Add(bar, 0, wxEXPAND | wxLEFT | wxRIGHT, 24);
}

void WarehouseWidget::loadActionIcons() {
    iconList_ = new wxImageList(22, 22, true, 3);
    iconList_->Add(bmpEdit_);
    iconList_->Add(bmpWriteOff_);
    iconList_->Add(bmpDelete_);
}

void WarehouseWidget::buildTable(wxSizer* root) {
    auto& tm = ThemeManager::instance();

    loadActionIcons();

    table_ = new ClickableListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 500),
                                   wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_NONE);
    table_->SetBackgroundColour(tm.surface());
    table_->SetForegroundColour(tm.text());
    table_->SetFont(wxFont(tm.fontSizeSmall() + 1, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    table_->AssignImageList(iconList_, wxIMAGE_LIST_SMALL);

    table_->AppendColumn(utf8::U("АРТИКУЛ"),      wxLIST_FORMAT_LEFT, 130);
    table_->AppendColumn(utf8::U("НАИМЕНОВАНИЕ"), wxLIST_FORMAT_LEFT, 340);
    table_->AppendColumn(utf8::U("ОСТАТОК"),      wxLIST_FORMAT_RIGHT, 110);
    table_->AppendColumn(utf8::U("МИН."),         wxLIST_FORMAT_RIGHT, 90);
    table_->AppendColumn(utf8::U("ЦЕНА"),         wxLIST_FORMAT_RIGHT, 110);

    bool canWrite = canWriteWarehouse(user_.role);
    if (canWrite) {
        table_->AppendColumn(utf8::U("РЕД."),  wxLIST_FORMAT_CENTER, 70);
        table_->AppendColumn(utf8::U("СПИС."), wxLIST_FORMAT_CENTER, 70);
        table_->AppendColumn(utf8::U("УДАЛ."), wxLIST_FORMAT_CENTER, 70);
    }

    table_->setOnLeftClick([this](int row, int col) {
        onTableClick(row, col);
    });

    root->Add(table_, 1, wxEXPAND | wxLEFT | wxRIGHT, 24);
}

void WarehouseWidget::buildFooter(wxSizer* root) {
    auto& tm = ThemeManager::instance();

    auto* footer = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 44));
    footer->SetBackgroundColour(tm.surface());

    auto* sizer = new wxBoxSizer(wxHORIZONTAL);

    footerCount_ = new wxStaticText(footer, wxID_ANY, "");
    footerCount_->SetForegroundColour(tm.muted());
    footerCount_->SetBackgroundColour(tm.surface());
    footerCount_->SetFont(wxFont(tm.fontSizeSmall(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

    footerSum_ = new wxStaticText(footer, wxID_ANY, "");
    footerSum_->SetForegroundColour(tm.text());
    footerSum_->SetBackgroundColour(tm.surface());
    footerSum_->SetFont(wxFont(tm.fontSizeSmall() + 2, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));

    sizer->Add(footerCount_, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 16);
    sizer->Add(footerSum_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);

    footer->SetSizer(sizer);
    root->Add(footer, 0, wxEXPAND | wxLEFT | wxRIGHT, 24);
}

void WarehouseWidget::loadParts() {
    if (!table_) return;

    allParts_ = PartRepository::instance().getAll();
    applyFilter();
    refreshAlert();
}

void WarehouseWidget::applyFilter() {
    if (!table_) return;

    auto& tm = ThemeManager::instance();
    table_->DeleteAllItems();
    filteredParts_.clear();

    wxString query = search_ ? search_->GetValue().Trim().Trim(false) : wxString();
    std::string queryUtf8 = std::string(query.ToUTF8().data());
    std::string queryLower = toLower(queryUtf8);

    bool canWrite = canWriteWarehouse(user_.role);

    long idx = 0;
    double totalSum = 0.0;
    int shownCount = 0;

    for (auto& p : allParts_) {
        bool matches = queryLower.empty();

        if (!matches) {
            std::string nameLower = toLower(p.name);
            std::string articleLower = toLower(p.article);

            if (nameLower.find(queryLower) != std::string::npos ||
                articleLower.find(queryLower) != std::string::npos) {
                matches = true;
            }
        }

        if (!matches) continue;

        long row = table_->InsertItem(idx, utf8::U(p.article.c_str()));
        table_->SetItem(row, 1, utf8::U(p.name.c_str()));
        table_->SetItem(row, 2, wxString::Format("%d", p.quantity));
        table_->SetItem(row, 3, wxString::Format("%d", p.minQuantity));
        table_->SetItem(row, 4, wxString::Format("%.0f", p.price));

        if (canWrite) {
            table_->SetItemColumnImage(row, 5, 0);
            table_->SetItemColumnImage(row, 6, 1);
            table_->SetItemColumnImage(row, 7, 2);
        }

        table_->SetItemData(row, p.id);

        if (p.quantity <= p.minQuantity) {
            table_->SetItemTextColour(row, wxColour(0xEF, 0x44, 0x44));
        } else {
            table_->SetItemTextColour(row, tm.text());
        }

        totalSum += p.price * p.quantity;
        filteredParts_.push_back(p);
        idx++;
        shownCount++;
    }

    if (footerCount_) {
        if (queryLower.empty()) {
            footerCount_->SetLabel(utf8::U("Позиций: ") +
                                   wxString::Format("%zu", allParts_.size()));
        } else {
            footerCount_->SetLabel(utf8::U("Найдено: ") +
                                   wxString::Format("%d", shownCount) +
                                   utf8::U(" из ") +
                                   wxString::Format("%zu", allParts_.size()));
        }
    }
    if (footerSum_) {
        footerSum_->SetLabel(utf8::U("Сумма склада: ") +
                             wxString::Format("%.0f", totalSum) +
                             utf8::U(" руб."));
    }
}

void WarehouseWidget::onSearchChanged(wxCommandEvent&) {
    applyFilter();
}

void WarehouseWidget::onAddPart(wxCommandEvent&) {
    if (!canWriteWarehouse(user_.role)) return;

    PartFormDialog dlg(this);
    if (dlg.ShowModal() != wxID_OK) return;

    Part part = dlg.getPart();
    if (!PartRepository::instance().add(part)) {
        wxMessageBox(utf8::U("Не удалось добавить позицию"),
                     utf8::U("Ошибка"), wxOK | wxICON_ERROR);
        return;
    }

    Logger::instance().log(user_.id, "Добавлена запчасть: " + part.name, "part", part.id);
    loadParts();
}

void WarehouseWidget::onTableClick(int row, int col) {
    if (row < 0 || col < 0) return;
    if (!canWriteWarehouse(user_.role)) return;

    long partId = table_->GetItemData(row);
    if (partId == 0) return;

    if (col == 5) {
        editPart(static_cast<int>(partId));
    } else if (col == 6) {
        writeOffPart(static_cast<int>(partId));
    } else if (col == 7) {
        deletePart(static_cast<int>(partId));
    }
}

void WarehouseWidget::editPart(int partId) {
    if (!canWriteWarehouse(user_.role)) return;

    Part part = PartRepository::instance().findById(partId);
    if (part.id == 0) return;

    PartFormDialog dlg(this, part);
    if (dlg.ShowModal() != wxID_OK) return;

    Part updated = dlg.getPart();
    if (!PartRepository::instance().update(updated)) {
        wxMessageBox(utf8::U("Не удалось сохранить изменения"),
                     utf8::U("Ошибка"), wxOK | wxICON_ERROR);
        return;
    }

    Logger::instance().log(user_.id, "Изменена запчасть: " + updated.name, "part", updated.id);
    loadParts();
}

void WarehouseWidget::deletePart(int partId) {
    if (!canWriteWarehouse(user_.role)) return;

    int answer = wxMessageBox(utf8::U("Удалить выбранную позицию?"),
                              utf8::U("Подтверждение"),
                              wxYES_NO | wxICON_QUESTION);
    if (answer != wxYES) return;

    if (!PartRepository::instance().remove(partId)) {
        wxMessageBox(utf8::U("Не удалось удалить позицию"),
                     utf8::U("Ошибка"), wxOK | wxICON_ERROR);
        return;
    }

    Logger::instance().log(user_.id, "Удалена запчасть id=" + std::to_string(partId), "part", partId);
    loadParts();
}

void WarehouseWidget::writeOffPart(int partId) {
    if (!canWriteWarehouse(user_.role)) return;

    Part part = PartRepository::instance().findById(partId);
    if (part.id == 0) return;

    wxString prompt = utf8::U("Сколько списать? Остаток: ") +
                      wxString::Format("%d", part.quantity);
    wxString value = wxGetTextFromUser(prompt, utf8::U("Списание"),
                                       "1", this);
    if (value.IsEmpty()) return;

    long qty = 0;
    if (!value.ToLong(&qty) || qty <= 0) {
        wxMessageBox(utf8::U("Некорректное количество"),
                     utf8::U("Ошибка"), wxOK | wxICON_ERROR);
        return;
    }

    if (!WarehouseService::instance().writeOffPart(partId, static_cast<int>(qty), 0, user_.id)) {
        wxMessageBox(utf8::U("Не удалось списать. Проверьте остаток."),
                     utf8::U("Ошибка"), wxOK | wxICON_ERROR);
        return;
    }
    loadParts();
}

TableData WarehouseWidget::buildTableData() {
    TableData td;
    td.title = utf8::toUtf8(utf8::U("Складские позиции"));
    td.headers = {
        utf8::toUtf8(utf8::U("Артикул")),
        utf8::toUtf8(utf8::U("Наименование")),
        utf8::toUtf8(utf8::U("Остаток")),
        utf8::toUtf8(utf8::U("Мин. остаток")),
        utf8::toUtf8(utf8::U("Цена, руб.")),
        utf8::toUtf8(utf8::U("Сумма, руб."))
    };

    for (auto& p : filteredParts_) {
        td.rows.push_back({
            p.article,
            p.name,
            std::to_string(p.quantity),
            std::to_string(p.minQuantity),
            wxString::Format("%.0f", p.price).ToStdString(),
            wxString::Format("%.0f", p.price * p.quantity).ToStdString()
        });
    }
    return td;
}

void WarehouseWidget::onExport(wxCommandEvent&) {
    TableData td = buildTableData();
    if (td.rows.empty()) {
        wxMessageBox(utf8::U("Нет данных для экспорта"),
                     utf8::U("Экспорт"), wxOK | wxICON_INFORMATION, this);
        return;
    }

    wxString fileName = wxFileSelector(
        utf8::U("Сохранить склад как CSV"), "", "warehouse.csv", "*.csv",
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

void WarehouseWidget::onPrint(wxCommandEvent&) {
    TableData td = buildTableData();
    if (td.rows.empty()) {
        wxMessageBox(utf8::U("Нет данных для печати"),
                     utf8::U("Печать"), wxOK | wxICON_INFORMATION, this);
        return;
    }

    auto& cfg = ConfigManager::instance();
    std::string outDir = std::string(
        wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath().ToUTF8().data());
    std::string path = outDir + "/warehouse_" + TableExport::timestamp() + ".pdf";

    std::string line1 = utf8::toUtf8(utf8::U("Всего позиций: ")) +
                        std::to_string(td.rows.size());

    if (TableExport::printPdf(td, path, cfg.fontPath(), line1, "")) {
        wxLaunchDefaultApplication(path);
    } else {
        wxMessageBox(utf8::U("Не удалось создать PDF"), utf8::U("Ошибка"),
                     wxOK | wxICON_ERROR, this);
    }
}

}