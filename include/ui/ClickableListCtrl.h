#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <functional>

namespace remont {

class ClickableListCtrl : public wxListCtrl {
public:
    ClickableListCtrl(wxWindow* parent, wxWindowID id,
                      const wxPoint& pos, const wxSize& size, long style);

    void setOnLeftClick(std::function<void(int row, int col)> cb);

private:
    void onLeftDown(wxMouseEvent& event);

    std::function<void(int, int)> onLeftClick_;
};

}