#ifndef PROCESSVIEW_H
#define PROCESSVIEW_H

#include <View.h>
#include <Locker.h>
#include <String.h>
#include <PopUpMenu.h>
#include <map>
#include <kernel/OS.h>

class BColumnListView;
class BMenuItem;

struct ProcessInfo {
    team_id id;
    BString name;
    BString path;
    BString userName;
    uint32 threadCount;
    uint32 areaCount;
    uid_t userID;
    uint64 memoryUsageBytes;
    float cpuUsage;
    bigtime_t totalUserTime;
    bigtime_t totalKernelTime;
    uint64 totalNetSent;
    uint64 totalNetRecv;
    uint64 totalDiskRead;
    uint64 totalDiskWrite;
};

class ProcessView : public BView {
public:
    ProcessView(BRect frame);
    virtual ~ProcessView();
    
    virtual void AttachedToWindow();
    virtual void MessageReceived(BMessage* message);
    virtual void Pulse();

private:
    void UpdateData();
    void KillSelectedProcess();
    void ShowContextMenu(BPoint screenPoint);
    BString FormatBytes(uint64 bytes);
    BString GetUserName(uid_t uid);
    
    BColumnListView* fProcessListView;
    BPopUpMenu* fContextMenu;
    
    std::map<team_id, ProcessInfo> fProcessTimeMap;
    std::map<thread_id, bigtime_t> fThreadTimeMap;
    bigtime_t fLastPulseSystemTime;
    
    BLocker fLocker;
};

#endif // PROCESSVIEW_H