#ifndef NETWORKVIEW_H
#define NETWORKVIEW_H

#include <View.h>
#include <Locker.h>
#include <String.h>
#include <map>
#include <string>

class BColumnListView;

struct InterfaceStatsRecord {
    uint64 bytesSent = 0;
    uint64 bytesReceived = 0;
    bigtime_t lastUpdateTime = 0;
};

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
    
    BLocker fLocker;
    std::map<std::string, InterfaceStatsRecord> fPreviousStatsMap;
};

#endif // NETWORKVIEW_H