#ifndef TEXTCONTROL_H
#define TEXTCONTROL_H

#include "View.h"

class BTextControl : public BView {
public:
    BTextControl(BRect frame, const char* name, const char* label, const char* text, BMessage* message, uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP, uint32 flags = B_WILL_DRAW | B_NAVIGABLE) : BView(frame, name, resizingMode, flags) {}
};

#endif
