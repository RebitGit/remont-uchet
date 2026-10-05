#pragma once
#include <wx/wx.h>

namespace remont {

class RoundedLogo : public wxPanel {
public:
    RoundedLogo(wxWindow* parent, int size = 56);

private:
    void onPaint(wxPaintEvent& event);
    int size_;
};

}