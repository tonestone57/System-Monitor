#ifndef MOCK_HANDLER_H
#define MOCK_HANDLER_H
class BMessage;
class BHandler {
public:
    virtual void MessageReceived(BMessage*) {}
};
#endif
