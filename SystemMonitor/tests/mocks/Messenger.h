#ifndef MOCK_MESSENGER_H
#define MOCK_MESSENGER_H
#include "../HaikuMocks.h"

class BMessenger {
public:
    BMessenger(BHandler*) {}
    void SendMessage(BMessage*) {}
};

#endif
