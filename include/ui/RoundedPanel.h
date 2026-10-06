#pragma once
#include <wx/wx.h>

namespace remont {

class RoundedPanel : public wxPanel {
public:
    RoundedPanel(wxWindow* parent, const wxColour& bg, int radius,
                 const wxSize& size = wxDefaultSize);

    void SetFillColour(const wxColour& c);
    void SetHoverColour(const wxColour& c);
    void SetIcon(const wxBitmap& icon);
    void SetLabel(const wxString& text, const wxColour& fg, int fontSize);
    void SetActive(bool active);

private:
    void onPaint(wxPaintEvent& event);
    void onEnter(wxMouseEvent& event);
    void onLeave(wxMouseEvent& event);

    wxColour bg_;
    wxColour hoverBg_;
    wxColour currentBg_;
    int radius_;
    wxBitmap icon_;
    wxString label_;
    wxColour labelFg_;
    int labelSize_ = 0;
    bool hovered_ = false;
    bool isActive_ = false;
};

}