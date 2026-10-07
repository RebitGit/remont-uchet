#include "ui/ActionRenderer.h"
#include <wx/dc.h>

namespace remont {

ActionRenderer::ActionRenderer(std::function<void(int)> onEdit,
                               std::function<void(int)> onWriteOff,
                               std::function<void(int)> onDelete)
    : wxDataViewCustomRenderer("long", wxDATAVIEW_CELL_ACTIVATABLE, wxALIGN_LEFT),
      onEdit_(std::move(onEdit)),
      onWriteOff_(std::move(onWriteOff)),
      onDelete_(std::move(onDelete))
{
}

void ActionRenderer::SetIcons(const wxBitmap& edit, const wxBitmap& writeOff,
                              const wxBitmap& del) {
    edit_ = edit;
    writeOff_ = writeOff;
    del_ = del;
}

bool ActionRenderer::SetValue(const wxVariant& value) {
    partId_ = static_cast<int>(value.GetLong());
    return true;
}

bool ActionRenderer::GetValue(wxVariant& value) const {
    value = wxVariant(static_cast<long>(partId_));
    return true;
}

wxSize ActionRenderer::GetSize() const {
    return wxSize(110, 28);
}

wxRect ActionRenderer::iconRect(const wxRect& cell, int index) const {
    int iconSize = 22;
    int gap = 8;
    int totalW = iconSize * 3 + gap * 2;
    int startX = cell.x + (cell.width - totalW) / 2;
    int y = cell.y + (cell.height - iconSize) / 2;
    int x = startX + index * (iconSize + gap);
    return wxRect(x, y, iconSize, iconSize);
}

bool ActionRenderer::Render(wxRect cell, wxDC* dc, int) {
    if (edit_.IsOk())     dc->DrawBitmap(edit_, iconRect(cell, 0).GetPosition());
    if (writeOff_.IsOk()) dc->DrawBitmap(writeOff_, iconRect(cell, 1).GetPosition());
    if (del_.IsOk())      dc->DrawBitmap(del_, iconRect(cell, 2).GetPosition());
    return true;
}

bool ActionRenderer::ActivateCell(const wxRect& cell, wxDataViewModel*,
                                  const wxDataViewItem&, unsigned int,
                                  const wxMouseEvent* mouseEvent) {
    if (!mouseEvent) return false;

    wxPoint pos = mouseEvent->GetPosition();

    if (iconRect(cell, 0).Contains(pos)) {
        if (onEdit_) onEdit_(partId_);
        return true;
    }
    if (iconRect(cell, 1).Contains(pos)) {
        if (onWriteOff_) onWriteOff_(partId_);
        return true;
    }
    if (iconRect(cell, 2).Contains(pos)) {
        if (onDelete_) onDelete_(partId_);
        return true;
    }
    return false;
}

}