#ifndef PROCESSVIEW_H
#define PROCESSVIEW_H

#include <View.h>
#include <Locker.h>
#include <String.h>
#include <PopUpMenu.h>
#include <TextControl.h>
#include <Message.h>
#include <map>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <atomic>
#include <kernel/OS.h>

class BListView;
class BMenuItem;
class BListItem;

#include <kernel/OS.h>

struct ProcessInfo {
    team_id id;
    char name[B_OS_NAME_LENGTH];
    char userName[B_OS_NAME_LENGTH];
    char state[32];
    uint32 threadCount;
    uint32 areaCount;
    uid_t userID;
    uint64 memoryUsageBytes;
    float cpuUsage;
};


const uint32 MSG_PROCESS_DATA_UPDATE = 'pdup';
const uint32 MSG_SEARCH_UPDATED = 'srch';

class ProcessListItem; // Forward declaration

class ProcessView : public BView {
public:
    ProcessView();
    virtual ~ProcessView();
    
    virtual void AttachedToWindow();
    virtual void DetachedFromWindow();
    virtual void MessageReceived(BMessage* message);
    virtual void KeyDown(const char* bytes, int32 numBytes);
    virtual void Hide();
    virtual void Show();

    void SaveState(BMessage& state);
    void LoadState(const BMessage& state);
    void SetRefreshInterval(bigtime_t interval);

private:
    static int32 UpdateThread(void* data);
    void Update(BMessage* message);
    void FilterRows();

    void KillSelectedProcess();
    void SuspendSelectedProcess();
    void ResumeSelectedProcess();
    void SetSelectedProcessPriority(int32 priority);
    void ShowContextMenu(BPoint screenPoint);
    BString GetUserName(uid_t uid, std::vector<char>& buffer);

    BListView* fProcessListView;
    BPopUpMenu* fContextMenu;
    BTextControl* fSearchControl;
    
    std::unordered_map<thread_id, bigtime_t> fThreadTimeMap;
    std::unordered_map<team_id, ProcessListItem*> fTeamItemMap;
    // std::unordered_set<ProcessListItem*> fVisibleItems; // Not easily tracked with BListView unless we manage addition/removal manually

    // Optimization members
    std::unordered_set<uid_t> fActiveUIDs;
    std::unordered_set<team_id> fActivePIDs;
    BString fStrRunning;
    BString fStrReady;
    BString fStrSleeping;

    // Buffers for filtering to avoid reallocation
    BString fFilterName; // Reused buffer for name filtering
    BString fFilterID;   // Reused buffer for ID filtering

    BLocker fCacheLock;
    std::unordered_map<uid_t, BString> fUserNameCache;
    bigtime_t fLastSystemTime;
    std::atomic<bigtime_t> fRefreshInterval;
    
    thread_id fUpdateThread;
    sem_id fQuitSem;
    std::atomic<bool> fTerminated;
    std::atomic<bool> fIsHidden;
};

#endif // PROCESSVIEW_H
