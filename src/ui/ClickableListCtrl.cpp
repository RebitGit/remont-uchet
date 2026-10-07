#include "ui/ClickableListCtrl.h"

namespace remont {

ClickableListCtrl::ClickableListCtrl(wxWindow* parent, wxWindowID id,
                                     const wxPoint& pos, const wxSize& size,
                                     long style)
    : wxListCtrl(parent, id, pos, size, style)
{
    Bind(wxEVT_LEFT_DOWN, &ClickableListCtrl::onLeftDown, this);
}

void ClickableListCtrl::setOnLeftClick(std::function<void(int, int)> cb) {
    onLeftClick_ = std::move(cb);
}

void ClickableListCtrl::onLeftDown(wxMouseEvent& event) {
    wxPoint pos = event.GetPosition();

    int flags = 0;
    long row = HitTest(pos, flags);

    if (row != wxNOT_FOUND) {
        int col = -1;
        int acc = 0;
        for (int i = 0; i < GetColumnCount(); ++i) {
            int w = GetColumnWidth(i);
            if (pos.x >= acc && pos.x < acc + w) {
                col = i;
                break;
            }
            acc += w;
        }

        if (col >= 0 && onLeftClick_) {
            onLeftClick_(static_cast<int>(row), col);
        }
    }

    event.Skip();
}

}