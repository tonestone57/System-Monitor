#ifndef NETWORKVIEW_H
#define NETWORKVIEW_H

#include <View.h>
#include <Locker.h>
#include <String.h>
#include <map>
#include <string>
#include <atomic>
#include "ActivityGraphView.h"

class BColumnListView;
class BRow;
class ActivityGraphView;

struct InterfaceStatsRecord {
    uint64 bytesSent = 0;
    uint64 bytesReceived = 0;
    bigtime_t lastUpdateTime = 0;
};

struct NetworkInfo {
    char name[B_OS_NAME_LENGTH];
    char typeStr[64];
    char addressStr[128];
    uint64 bytesSent;
    uint64 bytesReceived;
    bool hasStats;
};

const uint32 kMsgNetworkDataUpdate = 'netd';

class NetworkView : public BView {
public:
    NetworkView();
    virtual ~NetworkView();
    
    virtual void AttachedToWindow();
    virtual void DetachedFromWindow();
    virtual void MessageReceived(BMessage* message);
    virtual void Pulse();

    float GetUploadSpeed();
    float GetDownloadSpeed();
    void SetRefreshInterval(bigtime_t interval);

private:
    static int32 UpdateThread(void* data);
    void UpdateData(BMessage* message);
    BString FormatBytes(uint64 bytes);
    
    BColumnListView* fInterfaceListView;
    ActivityGraphView* fDownloadGraph;
    ActivityGraphView* fUploadGraph;
    
    BLocker fLocker;

    struct BStringLess {
        bool operator()(const BString& a, const BString& b) const {
            return a.Compare(b) < 0;
        }
    };

    std::map<BString, InterfaceStatsRecord, BStringLess> fPreviousStatsMap;
    std::map<BString, BRow*, BStringLess> fInterfaceRowMap;
    float fUploadSpeed;
    float fDownloadSpeed;

    thread_id fUpdateThread;
    sem_id fScanSem;
    std::atomic<bool> fTerminated;
};

#endif // NETWORKVIEW_H
