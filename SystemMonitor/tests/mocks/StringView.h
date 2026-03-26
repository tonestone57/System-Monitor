#ifndef MOCK_STRING_VIEW_H
#define MOCK_STRING_VIEW_H
#include "../HaikuMocks.h"
#include "View.h"
class BStringView : public BView {
public:
    BStringView(const char* name, const char* text) : BView(name, 0) {}
    void SetAlignment(int) {}
    void SetFont(const BFont*) {}
    void SetText(const char*) {}
};
#endif
