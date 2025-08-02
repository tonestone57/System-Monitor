#include "GraphView.h"
#include <Bitmap.h>
#include <View.h>

GraphView::GraphView(const char* name, rgb_color color, size_t maxSamples)
    : BView(name, B_WILL_DRAW),
      fBitmap(NULL),
      fOffscreenView(NULL),
      fColor(color),
      fMaxSamples(maxSamples)
{
    SetViewColor(B_TRANSPARENT_COLOR);
}

GraphView::~GraphView()
{
    delete fBitmap;
}

void GraphView::AttachedToWindow()
{
    BView::AttachedToWindow();
    _InitBitmap();
}

void GraphView::_InitBitmap()
{
    delete fBitmap;
    BRect bounds = Bounds();
    fBitmap = new BBitmap(bounds, B_RGB32, true);
    if (fBitmap->InitCheck() != B_OK) {
        delete fBitmap;
        fBitmap = NULL;
        return;
    }

    fOffscreenView = new BView(bounds, "offscreen_view", B_FOLLOW_NONE, 0);
    fBitmap->AddChild(fOffscreenView);

    // Initial clear
    fBitmap->Lock();
    fOffscreenView->SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    fOffscreenView->FillRect(fOffscreenView->Bounds());
    fOffscreenView->Sync();
    fBitmap->Unlock();

    fHistory.clear();
}

void GraphView::FrameResized(float newWidth, float newHeight)
{
    BView::FrameResized(newWidth, newHeight);
    _InitBitmap();
}

void GraphView::AddSample(float percent)
{
    if (!fBitmap)
        return;

    if (fHistory.size() >= fMaxSamples)
        fHistory.pop_front();
    fHistory.push_back(std::max(0.0f, std::min(percent, 100.0f)));

    _Render();
    Invalidate();
}

void GraphView::_Render()
{
    if (fHistory.size() < 2)
        return;

    fBitmap->Lock();

    // Scroll left
    BRect bounds = fOffscreenView->Bounds();
    float dx = bounds.Width() / (fMaxSamples - 1);
    fOffscreenView->ScrollBy(-dx, 0);

    // Clear the newly exposed area
    BRect exposedRect(bounds.right - dx, 0, bounds.right, bounds.bottom);
    fOffscreenView->SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    fOffscreenView->FillRect(exposedRect);

    // Draw the new line segment
    fOffscreenView->SetHighColor(fColor);
    float height = bounds.Height();

    float y1 = height * (1.0f - fHistory[fHistory.size() - 2] / 100.0f);
    float y2 = height * (1.0f - fHistory.back() / 100.0f);

    BPoint p1(bounds.right - dx, y1);
    BPoint p2(bounds.right, y2);

    fOffscreenView->StrokeLine(p1, p2);
    fOffscreenView->Sync();

    fBitmap->Unlock();
}

void GraphView::Draw(BRect updateRect)
{
    if (fBitmap) {
        DrawBitmap(fBitmap, updateRect, updateRect);
    }
}
