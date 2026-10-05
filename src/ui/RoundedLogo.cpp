#include "ui/RoundedLogo.h"
#include "resources/styles.h"
#include <wx/dcbuffer.h>
#include <wx/graphics.h>

namespace remont {

RoundedLogo::RoundedLogo(wxWindow* parent, int size)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(size, size)),
      size_(size)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(parent->GetBackgroundColour());
    SetMinSize(wxSize(size, size));
    Bind(wxEVT_PAINT, &RoundedLogo::onPaint, this);
}

void RoundedLogo::onPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);

    dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
    dc.Clear();

    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
    if (!gc) return;

    gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);

    const double radius = 12.0;
    gc->SetBrush(wxBrush(styles::Primary));
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->DrawRoundedRectangle(0, 0, size_, size_, radius);

    const double cx = size_ / 2.0;
    const double cy = size_ / 2.0;

    const double outerSize = 22.0;
    const double outerLeft = cx - outerSize / 2.0;
    const double outerTop  = cy - outerSize / 2.0;

    const double innerSize = 10.0;
    const double innerLeft = cx - innerSize / 2.0;
    const double innerTop  = cy - innerSize / 2.0;

    wxPen whitePen(*wxWHITE, 2);
    gc->SetPen(whitePen);
    gc->SetBrush(*wxTRANSPARENT_BRUSH);

    gc->DrawRectangle(outerLeft, outerTop, outerSize, outerSize);

    gc->SetBrush(wxBrush(*wxWHITE));
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->DrawRectangle(innerLeft, innerTop, innerSize, innerSize);

    const double pin = 3.0;
    const double pinOffset = 5.5;

    gc->SetPen(whitePen);

    gc->StrokeLine(cx - pinOffset, outerTop, cx - pinOffset, outerTop - pin);
    gc->StrokeLine(cx + pinOffset, outerTop, cx + pinOffset, outerTop - pin);
    gc->StrokeLine(cx - pinOffset, outerTop + outerSize, cx - pinOffset, outerTop + outerSize + pin);
    gc->StrokeLine(cx + pinOffset, outerTop + outerSize, cx + pinOffset, outerTop + outerSize + pin);

    gc->StrokeLine(outerLeft, cy - pinOffset, outerLeft - pin, cy - pinOffset);
    gc->StrokeLine(outerLeft, cy + pinOffset, outerLeft - pin, cy + pinOffset);
    gc->StrokeLine(outerLeft + outerSize, cy - pinOffset, outerLeft + outerSize + pin, cy - pinOffset);
    gc->StrokeLine(outerLeft + outerSize, cy + pinOffset, outerLeft + outerSize + pin, cy + pinOffset);
}

}