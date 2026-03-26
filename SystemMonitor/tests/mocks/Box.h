#ifndef MOCK_BOX_H
#define MOCK_BOX_H
#include "../HaikuMocks.h"
#include "View.h"
class BBox : public BView {
public:
    BBox(const char* name) : BView(name, 0) {}
};
#endif
