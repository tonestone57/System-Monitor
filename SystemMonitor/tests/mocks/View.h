#ifndef MOCK_VIEW_H
#define MOCK_VIEW_H
#include "../HaikuMocks.h"
class BView {
public:
    BView(const char*, uint32) {}
    virtual ~BView() {}
    void SetViewColor(int) {}
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
};
#endif
