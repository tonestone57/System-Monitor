#ifndef MOCK_STRING_VIEW_H
#define MOCK_STRING_VIEW_H
#include "../HaikuMocks.h"
class BStringView {
public:
    BStringView() {}
    BStringView(const char*, const char*) {}
    void SetViewColor(int) {}
    BWindow* Window() { return nullptr; }
    void SetExplicitMinSize(BSize) {}
    void SetExplicitMaxSize(BSize) {}
    void InvalidateLayout() {}
    void SetAlignment(int) {}
    void SetFont(const BFont*) {}
};
#endif
