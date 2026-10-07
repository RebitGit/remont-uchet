#pragma once
#include <wx/dataview.h>
#include <wx/wx.h>

namespace remont {

class ActionRenderer : public wxDataViewCustomRenderer {
public:
    enum class Action { Edit, WriteOff, Delete };

    ActionRenderer(std::function<void(int)> onEdit,
                   std::function<void(int)> onWriteOff,
                   std::function<void(int)> onDelete);

    bool SetValue(const wxVariant& value) override;
    bool GetValue(wxVariant& value) const override;
    wxSize GetSize() const override;
    bool Render(wxRect cell, wxDC* dc, int state) override;
    bool ActivateCell(const wxRect& cell, wxDataViewModel* model,
                      const wxDataViewItem& item, unsigned int col,
                      const wxMouseEvent* mouseEvent) override;

    void SetIcons(const wxBitmap& edit, const wxBitmap& writeOff, const wxBitmap& del);

private:
    wxBitmap edit_;
    wxBitmap writeOff_;
    wxBitmap del_;
    int partId_ = 0;

    std::function<void(int)> onEdit_;
    std::function<void(int)> onWriteOff_;
    std::function<void(int)> onDelete_;

    wxRect iconRect(const wxRect& cell, int index) const;
};

}