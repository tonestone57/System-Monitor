#ifndef MOCK_VIEW_H
#define MOCK_VIEW_H
#include "../HaikuMocks.h"
class BView {
public:
    BView(const char* name, uint32 flags) {}
    virtual ~BView() {}
    void SetViewColor(int) {}
    void SetViewColor(rgb_color) {}
    uint32 Flags() const { return 0; }
    void SetFlags(uint32) {}
    bool IsHidden() const { return false; }
    BWindow* Window() { return nullptr; }
    void SetExplicitMinSize(BSize s) {}
    void SetExplicitMaxSize(BSize s) {}
    void InvalidateLayout() {}
    virtual void AttachedToWindow() {}
    virtual void Pulse() {}
    virtual void Draw(BRect) {}
    virtual void MouseDown(BPoint) {}
    void SetHighColor(rgb_color c) {}
    void FillRect(BRect r) {}
    void GetFontHeight(font_height* fh) const { MockGetFontHeight(fh); }
    float StringWidth(const char* s) const { return 10.0f; }
    void DrawString(const char* s, BPoint p) {}
    void Invalidate() {}
};
#endif
