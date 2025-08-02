#ifndef NETWORKVIEW_H
#define NETWORKVIEW_H

#include <View.h>
#include <Locker.h>
#include <String.h>
#include <map>
#include <string>
#include "ActivityGraphView.h"

class BColumnListView;
class ActivityGraphView;

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

    float GetUploadSpeed();
    float GetDownloadSpeed();

private:
    void UpdateData();
    BString FormatBytes(uint64 bytes);
    BString FormatSpeed(uint64 bytesDelta, bigtime_t microSecondsDelta);
    
    BColumnListView* fInterfaceListView;
    ActivityGraphView* fDownloadGraph;
    ActivityGraphView* fUploadGraph;
    
    BLocker fLocker;
    std::map<std::string, InterfaceStatsRecord> fPreviousStatsMap;
    float fUploadSpeed;
    float fDownloadSpeed;
};

#endif // NETWORKVIEW_H