#ifndef MOCK_LAYOUT_BUILDER_H
#define MOCK_LAYOUT_BUILDER_H
#include "../HaikuMocks.h"
#include "View.h"

#define B_VERTICAL 1
#define B_HORIZONTAL 2

class BLayoutBuilder {
public:
    template<typename T = BView>
    class Group {
    public:
        Group(T* view, int dir = 0) {}
        Group& SetInsets(int) { return *this; }
        Group& AddGroup(int) { return *this; }
        Group& Add(BView*) { return *this; }
        Group& Add(void*) { return *this; }
        Group& AddGlue() { return *this; }
        Group& End() { return *this; }
    };
};
#endif
