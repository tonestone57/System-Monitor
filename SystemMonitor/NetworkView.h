#ifndef NETWORKVIEW_H
#define NETWORKVIEW_H

#include <View.h>
#include <Locker.h>
#include <String.h>

class BColumnListView;

class NetworkView : public BView {
public:
    NetworkView(BRect frame);
    virtual ~NetworkView();
    
    virtual void AttachedToWindow();
    virtual void Pulse();

private:
    void UpdateData();
    BString FormatBytes(uint64 bytes);
    BString FormatSpeed(uint64 bytesDelta, bigtime_t microSecondsDelta);
    
    BColumnListView* fInterfaceListView;
    int fSocket;
    
    BLocker fLocker;
};

#endif // NETWORKVIEW_H