#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include "ui/ClickableListCtrl.h"
#include "models/User.h"
#include "models/Part.h"
#include "utils/TableExport.h"
#include <vector>

namespace remont {

class WarehouseWidget : public wxPanel {
public:
    WarehouseWidget(wxWindow* parent, const User& user);

private:
    void buildAlert(wxSizer* root);
    void buildToolbar(wxSizer* root);
    void buildTable(wxSizer* root);
    void buildFooter(wxSizer* root);
    void loadParts();
    void loadActionIcons();
    void applyFilter();
    void refreshAlert();
    wxBitmap loadIcon(const wxString& path, int size);

    void onAddPart(wxCommandEvent& event);
    void onSearchChanged(wxCommandEvent& event);
    void onTableClick(int row, int col);
    void onExport(wxCommandEvent& event);
    void onPrint(wxCommandEvent& event);

    void editPart(int partId);
    void deletePart(int partId);
    void writeOffPart(int partId);

    TableData buildTableData();

    User user_;

    wxSizer* mainSizer_ = nullptr;
    wxPanel* alertPanel_ = nullptr;
    wxStaticText* alertText_ = nullptr;

    ClickableListCtrl* table_ = nullptr;
    wxImageList* iconList_ = nullptr;
    wxTextCtrl* search_ = nullptr;
    wxStaticText* footerCount_ = nullptr;
    wxStaticText* footerSum_ = nullptr;

    wxBitmap bmpEdit_;
    wxBitmap bmpWriteOff_;
    wxBitmap bmpDelete_;

    std::vector<Part> allParts_;
    std::vector<Part> filteredParts_;
};

}