#ifndef MOCK_GRID_LAYOUT_H
#define MOCK_GRID_LAYOUT_H
#include "../HaikuMocks.h"
#include "View.h"
class BGridLayout {
public:
    BGridLayout(int, int) {}
    void AddView(BView*, int, int) {}
    void SetInsets(int, int, int, int) {}
    void SetColumnWeight(int, float) {}
};
#endif
