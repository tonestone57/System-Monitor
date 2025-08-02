#ifndef PROCESSVIEW_H
#define PROCESSVIEW_H

#include <View.h>
#include <Locker.h>
#include <String.h>
#include <PopUpMenu.h>
#include <map>
#include <vector>
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
};


const uint32 MSG_PROCESS_DATA_UPDATE = 'pdup';


class ProcessView : public BView {
public:
    ProcessView(BRect frame);
    virtual ~ProcessView();
    
    virtual void AttachedToWindow();
    virtual void DetachedFromWindow();
    virtual void MessageReceived(BMessage* message);

private:
    static int32 UpdateThread(void* data);
    void Update();

    void KillSelectedProcess();
    void ShowContextMenu(BPoint screenPoint);
    BString FormatBytes(uint64 bytes);
    BString GetUserName(uid_t uid);
    
    BColumnListView* fProcessListView;
    BPopUpMenu* fContextMenu;
    
    std::map<thread_id, bigtime_t> fThreadTimeMap;
    bigtime_t fLastSystemTime;
    
    thread_id fUpdateThread;
    volatile bool fTerminated;
};

#endif // PROCESSVIEW_H