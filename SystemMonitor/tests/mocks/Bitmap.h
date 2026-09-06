#ifndef MOCK_BITMAP_H
#define MOCK_BITMAP_H
#include "HaikuMocks.h"
class BBitmap { public: BBitmap(BRect bounds, int space) {} ~BBitmap() {} int InitCheck() const { return 0; } };
#endif
