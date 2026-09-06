#ifndef MOCK_VIEW_H
#define MOCK_VIEW_H
#include "../HaikuMocks.h"
typedef enum {
    B_ITEMS_IN_COLUMN
} menu_layout;

#define B_FOLLOW_LEFT 1
#define B_FOLLOW_TOP 2

class BView {
public:
    BView() {}
    BView(BRect frame, const char* name, uint32 resizingMode, uint32 flags) {}
    BView(const char* name, uint32 flags) {}
    virtual ~BView() {}
    void SetViewColor(int) {}
    void SetViewColor(rgb_color) {}
    void SetLowColor(rgb_color) {}
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
    void SetDrawingMode(int) {}
    void DrawBitmap(void* bitmap, BPoint p) {}
    void DrawBitmap(void* bitmap, BRect r) {}
    void Invalidate() {}
};
#endif
