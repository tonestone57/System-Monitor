#ifndef MOCK_MESSENGER_H
#define MOCK_MESSENGER_H
#include "../HaikuMocks.h"
class BMessenger {
public:
    BMessenger(BHandler*) {}
    int SendMessage(BMessage*) { return 0; }
};
#endif
