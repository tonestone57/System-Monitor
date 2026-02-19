#ifndef NETWORKVIEW_H
#define NETWORKVIEW_H

#include <View.h>
#include <Locker.h>
#include <String.h>
#include <map>
#include <string>
#include <set>
#include <atomic>
#include <Font.h>
#include "ActivityGraphView.h"

class BListView;
class BListItem;
class InterfaceListItem; // Forward declaration
class ActivityGraphView;

struct InterfaceStatsRecord {
    uint64 bytesSent = 0;
    uint64 bytesReceived = 0;
    bigtime_t lastUpdateTime = 0;
    int32 generation = 0;
};

struct NetworkInfo {
    char name[B_OS_NAME_LENGTH];
    char typeStr[64];
    char addressStr[128];
    uint64 bytesSent;
    uint64 bytesReceived;
    bool hasStats;
    bool isLoopback;
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
    void SetPerformanceViewVisible(bool visible) { fPerformanceViewVisible = visible; }

    float NameWidth() const { return fNameWidth; }
    float TypeWidth() const { return fTypeWidth; }
    float AddrWidth() const { return fAddrWidth; }
    float SentWidth() const { return fSentWidth; }
    float RecvWidth() const { return fRecvWidth; }
    float TxSpeedWidth() const { return fTxSpeedWidth; }
    float RxSpeedWidth() const { return fRxSpeedWidth; }

private:
    static int32 UpdateThread(void* data);
    void UpdateData(BMessage* message);
    
    BListView* fInterfaceListView;
    ActivityGraphView* fDownloadGraph;
    ActivityGraphView* fUploadGraph;
    
    BLocker fLocker;

    struct BStringLess {
        bool operator()(const BString& a, const BString& b) const {
            return a.Compare(b) < 0;
        }
    };

    std::map<BString, InterfaceStatsRecord, BStringLess> fPreviousStatsMap;
    std::map<BString, InterfaceListItem*, BStringLess> fInterfaceItemMap;
    float fUploadSpeed;
    float fDownloadSpeed;

    BFont fCachedFont;

    thread_id fUpdateThread;
    sem_id fScanSem;
    std::atomic<bool> fTerminated;
    std::atomic<bool> fPerformanceViewVisible;
    std::atomic<bigtime_t> fRefreshInterval;
    int32 fListGeneration;

    float fNameWidth;
    float fTypeWidth;
    float fAddrWidth;
    float fSentWidth;
    float fRecvWidth;
    float fTxSpeedWidth;
    float fRxSpeedWidth;
};

#endif // NETWORKVIEW_H
