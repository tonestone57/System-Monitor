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
#include <atomic>
#include <kernel/OS.h>

class BColumnListView;
class BMenuItem;
class BRow;
class BColumn;

#include <kernel/OS.h>

struct ProcessInfo {
    team_id id;
    char name[B_OS_NAME_LENGTH];
    char path[B_PATH_NAME_LENGTH];
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


class ProcessView : public BView {
public:
    ProcessView();
    virtual ~ProcessView();
    
    virtual void AttachedToWindow();
    virtual void DetachedFromWindow();
    virtual void MessageReceived(BMessage* message);
    virtual void MouseDown(BPoint where);
    virtual void KeyDown(const char* bytes, int32 numBytes);
    virtual void Hide();
    virtual void Show();

    void SaveState(BMessage& state);
    void LoadState(const BMessage& state);

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
    void SetRefreshInterval(bigtime_t interval);

    BColumnListView* fProcessListView;
    BPopUpMenu* fContextMenu;
    BTextControl* fSearchControl;

    BColumn* fPIDColumn;
    BColumn* fNameColumn;
    BColumn* fStateColumn;
    BColumn* fCPUColumn;
    BColumn* fMemColumn;
    BColumn* fThreadsColumn;
    BColumn* fUserColumn;
    
    std::unordered_map<thread_id, bigtime_t> fThreadTimeMap;
	std::unordered_map<team_id, BRow*> fTeamRowMap;
    BLocker fCacheLock;
    std::unordered_map<uid_t, BString> fUserNameCache;
    bigtime_t fLastSystemTime;
    bigtime_t fRefreshInterval;
    
    thread_id fUpdateThread;
    sem_id fQuitSem;
    std::atomic<bool> fTerminated;
    std::atomic<bool> fIsHidden;
};

#endif // PROCESSVIEW_H
