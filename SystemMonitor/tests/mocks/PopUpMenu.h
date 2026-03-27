#ifndef POPUPMENU_H
#define POPUPMENU_H

#include "View.h"

class BPopUpMenu : public BView {
public:
    BPopUpMenu(const char* name, bool radioMode = true, bool labelFromMarked = true, menu_layout layout = B_ITEMS_IN_COLUMN) : BView(BRect(), name, 0, 0) {}
};

#endif
