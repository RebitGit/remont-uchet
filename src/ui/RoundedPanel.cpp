#include "ui/RoundedPanel.h"
#include <wx/dcbuffer.h>
#include <wx/graphics.h>

namespace remont {

RoundedPanel::RoundedPanel(wxWindow* parent, const wxColour& bg, int radius,
                           const wxSize& size)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, size),
      bg_(bg),
      hoverBg_(bg),
      currentBg_(bg),
      radius_(radius)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(parent->GetBackgroundColour());
    SetMinSize(size);

    Bind(wxEVT_PAINT, &RoundedPanel::onPaint, this);
    Bind(wxEVT_ENTER_WINDOW, &RoundedPanel::onEnter, this);
    Bind(wxEVT_LEAVE_WINDOW, &RoundedPanel::onLeave, this);
}

void RoundedPanel::SetFillColour(const wxColour& c) {
    bg_ = c;
    if (!hovered_ || isActive_) {
        currentBg_ = bg_;
    } else {
        currentBg_ = hoverBg_;
    }
    Refresh();
    Update();
}

void RoundedPanel::SetHoverColour(const wxColour& c) {
    hoverBg_ = c;
    if (hovered_ && !isActive_) {
        currentBg_ = hoverBg_;
        Refresh();
    }
}

void RoundedPanel::SetIcon(const wxBitmap& icon) {
    icon_ = icon;
    Refresh();
}

void RoundedPanel::SetLabel(const wxString& text, const wxColour& fg, int fontSize) {
    label_ = text;
    labelFg_ = fg;
    labelSize_ = fontSize;
    Refresh();
}

void RoundedPanel::SetActive(bool active) {
    isActive_ = active;
    if (isActive_) {
        currentBg_ = bg_;
    } else if (hovered_) {
        currentBg_ = hoverBg_;
    } else {
        currentBg_ = bg_;
    }
    Refresh();
}

void RoundedPanel::onEnter(wxMouseEvent& event) {
    hovered_ = true;
    if (!isActive_) {
        currentBg_ = hoverBg_;
        Refresh();
    }
    event.Skip();
}

void RoundedPanel::onLeave(wxMouseEvent& event) {
    hovered_ = false;
    currentBg_ = bg_;
    Refresh();
    event.Skip();
}

void RoundedPanel::onPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
    dc.Clear();

    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
    if (!gc) return;

    gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);

    wxSize s = GetSize();

    gc->SetBrush(wxBrush(currentBg_));
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->DrawRoundedRectangle(0, 0, s.GetWidth(), s.GetHeight(), radius_);

    if (label_.IsEmpty()) {
        if (icon_.IsOk()) {
            int ix = (s.GetWidth() - icon_.GetWidth()) / 2;
            int iy = (s.GetHeight() - icon_.GetHeight()) / 2;
            gc->DrawBitmap(icon_, ix, iy, icon_.GetWidth(), icon_.GetHeight());
        }
        return;
    }

    int textX = 16;

    if (icon_.IsOk()) {
        int ix = 16;
        int iy = (s.GetHeight() - icon_.GetHeight()) / 2;
        gc->DrawBitmap(icon_, ix, iy, icon_.GetWidth(), icon_.GetHeight());
        textX = ix + icon_.GetWidth() + 12;
    }

    gc->SetFont(wxFont(labelSize_, wxFONTFAMILY_DEFAULT,
                       wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL), labelFg_);
    double tw, th;
    gc->GetTextExtent(label_, &tw, &th);
    gc->DrawText(label_, textX, (s.GetHeight() - th) / 2.0);
}

}