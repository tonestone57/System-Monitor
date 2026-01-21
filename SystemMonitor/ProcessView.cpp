#include "ProcessView.h"
#include "Utils.h"
#include "MonitorColumnTypes.h"
#include <LayoutBuilder.h>
#include <private/interface/ColumnListView.h>
#include <private/interface/ColumnTypes.h>
#include <Button.h>
#include <kernel/image.h>
#include <pwd.h>
#include <signal.h>
#include <Alert.h>
#include <Roster.h>
#include <Path.h>
#include <Box.h>
#include <cstdio>
#include <MenuItem.h>
#include <Font.h>
#include <vector>
#include <unordered_set>
#include <Window.h>
#include <Invoker.h>
#include <Messenger.h>
#include <Catalog.h>
#include <Autolock.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ProcessView"


namespace {

class ProcessListView : public BColumnListView {
public:
    ProcessListView(BRect rect, const char* name, uint32 resizingMode, uint32 flags,
        border_style border = B_NO_BORDER, bool showHorizontalScrollbar = true)
        : BColumnListView(rect, name, resizingMode, flags, border, showHorizontalScrollbar),
          fSortColumn(NULL),
          fSortInverse(false)
    {
    }

    virtual void MouseDown(BPoint where) {
        BMessage* msg = Window()->CurrentMessage();
        int32 buttons = 0;
        msg->FindInt32("buttons", &buttons);
        if (buttons & B_SECONDARY_MOUSE_BUTTON) {
            // Context menu logic
            // We need to notify the parent ProcessView
            // But we don't have a direct pointer easily accessible unless we store it.
            // BColumnListView doesn't propagate right clicks automatically in all cases.
            // However, since ProcessView is the target of the list view's invocation,
            // we can try sending a message to the window or view.

            // Best approach: Send a message to the target (ProcessView)
            BMessenger messenger(Target());
            if (messenger.IsValid()) {
                BMessage contextMsg('cntx'); // MSG_SHOW_CONTEXT_MENU
                BPoint screenWhere = where;
                ConvertToScreen(&screenWhere);
                contextMsg.AddPoint("screen_where", screenWhere);
                messenger.SendMessage(&contextMsg);
            }
        }
        BColumnListView::MouseDown(where);
    }

    virtual void SetSortColumn(BColumn* column, bool add, bool inverse) {
        if (!add) {
            fSortColumn = column;
            fSortInverse = inverse;
        }
        BColumnListView::SetSortColumn(column, add, inverse);
    }

    void GetSortColumn(BColumn** column, bool* inverse, bool* sensitive) {
        if (column) *column = fSortColumn;
        if (inverse) *inverse = fSortInverse;
        if (sensitive) *sensitive = true;
    }

    virtual void KeyDown(const char* bytes, int32 numBytes) {
        if (numBytes == 1 && bytes[0] == B_DELETE) {
            BMessenger messenger(Target());
            if (messenger.IsValid())
                messenger.SendMessage(MSG_KILL_PROCESS);
            return;
        }
        BColumnListView::KeyDown(bytes, numBytes);
    }

private:
    BColumn* fSortColumn;
    bool fSortInverse;
};

} // namespace

enum {
    kPIDColumn,
    kProcessNameColumn,
    kStateColumn,
    kCPUUsageColumn,
    kMemoryUsageColumn,
    kThreadCountColumn,
    kUserNameColumn
};

const uint32 MSG_KILL_PROCESS = 'kill';
const uint32 MSG_SUSPEND_PROCESS = 'susp';
const uint32 MSG_RESUME_PROCESS = 'resm';
const uint32 MSG_PRIORITY_LOW = 'pril';
const uint32 MSG_PRIORITY_NORMAL = 'prin';
const uint32 MSG_PRIORITY_HIGH = 'prih';
const uint32 MSG_SHOW_CONTEXT_MENU = 'cntx';
const uint32 MSG_CONFIRM_KILL = 'conf';

ProcessView::ProcessView()
    : BView("ProcessView", B_WILL_DRAW),
      fLastSystemTime(0),
      fRefreshInterval(1000000),
      fUpdateThread(B_ERROR),
      fTerminated(false),
      fIsHidden(false)
{
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

    fQuitSem = create_sem(0, "ProcessView Quit");

    fSearchControl = new BTextControl("Search", B_TRANSLATE("Search:"), "", new BMessage(MSG_SEARCH_UPDATED));
    fSearchControl->SetModificationMessage(new BMessage(MSG_SEARCH_UPDATED));

    fProcessListView = new ProcessListView(Bounds(), "process_clv",
                                           B_FOLLOW_ALL_SIDES,
                                           B_WILL_DRAW | B_NAVIGABLE,
                                           B_PLAIN_BORDER, true);

    fPIDColumn = new BIntegerColumn(B_TRANSLATE("PID"), 60, 30, 100);
    fProcessListView->AddColumn(fPIDColumn, kPIDColumn);

    fNameColumn = new BStringColumn(B_TRANSLATE("Name"), 180, 50, 500, B_TRUNCATE_END);
    fProcessListView->AddColumn(fNameColumn, kProcessNameColumn);

    fCPUColumn = new BFloatColumn(B_TRANSLATE("Total CPU %"), 90, 50, 120, B_TRUNCATE_END, B_ALIGN_RIGHT);
    fProcessListView->AddColumn(fCPUColumn, kCPUUsageColumn);

    fMemColumn = new BSizeColumn(B_TRANSLATE("Memory"), 100, 50, 200, B_TRUNCATE_END, B_ALIGN_RIGHT);
    fProcessListView->AddColumn(fMemColumn, kMemoryUsageColumn);

    fThreadsColumn = new BIntegerColumn(B_TRANSLATE("Threads"), 80, 40, 120, B_ALIGN_RIGHT);
    fProcessListView->AddColumn(fThreadsColumn, kThreadCountColumn);

    fStateColumn = new BStringColumn(B_TRANSLATE("State"), 80, 40, 150, B_TRUNCATE_END);
    fProcessListView->AddColumn(fStateColumn, kStateColumn);

    fUserColumn = new BStringColumn(B_TRANSLATE("User"), 80, 40, 150, B_TRUNCATE_END);
    fProcessListView->AddColumn(fUserColumn, kUserNameColumn);

    fProcessListView->SetSortColumn(fCPUColumn, false, false);

    // Cache translated strings
    fStrRunning = B_TRANSLATE("Running");
    fStrReady = B_TRANSLATE("Ready");
    fStrSleeping = B_TRANSLATE("Sleeping");

    fContextMenu = new BPopUpMenu("ProcessContext", false, false);
    fContextMenu->AddItem(new BMenuItem(B_TRANSLATE("Kill Process"), new BMessage(MSG_KILL_PROCESS)));
    fContextMenu->AddSeparatorItem();
    fContextMenu->AddItem(new BMenuItem(B_TRANSLATE("Suspend"), new BMessage(MSG_SUSPEND_PROCESS)));
    fContextMenu->AddItem(new BMenuItem(B_TRANSLATE("Resume"), new BMessage(MSG_RESUME_PROCESS)));

    BMenu* priorityMenu = new BMenu(B_TRANSLATE("Set Priority"));
    priorityMenu->AddItem(new BMenuItem(B_TRANSLATE("Low"), new BMessage(MSG_PRIORITY_LOW)));
    priorityMenu->AddItem(new BMenuItem(B_TRANSLATE("Normal"), new BMessage(MSG_PRIORITY_NORMAL)));
    priorityMenu->AddItem(new BMenuItem(B_TRANSLATE("High"), new BMessage(MSG_PRIORITY_HIGH)));
    fContextMenu->AddItem(priorityMenu);

    BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
        .SetInsets(0)
        .Add(fSearchControl)
        .Add(fProcessListView)
    .End();
}

ProcessView::~ProcessView()
{
    fTerminated = true;
    if (fQuitSem >= 0)
        delete_sem(fQuitSem);
    if (fUpdateThread != B_ERROR) {
        status_t ret;
        wait_for_thread(fUpdateThread, &ret);
    }
    delete fContextMenu;

    // Clean up BRow objects.
    // Rows currently in the list view are owned by it and will be deleted by it.
    // We must only manually delete rows that are hidden (not in the list view).
    for (auto& pair : fTeamRowMap) {
        if (fVisibleRows.find(pair.second) == fVisibleRows.end()) {
            delete pair.second;
        }
    }
    fTeamRowMap.clear();
    fVisibleRows.clear();
}

void ProcessView::AttachedToWindow()
{
    BView::AttachedToWindow();
    fTerminated = false;
    fProcessListView->SetTarget(this);
    fSearchControl->SetTarget(this);
    fLastSystemTime = system_time();

    if (fQuitSem < 0)
        fQuitSem = create_sem(0, "ProcessView Quit");

    // Clear thread time map to avoid stale calculations
    fThreadTimeMap.clear();

    fUpdateThread = spawn_thread(UpdateThread, "Process Update", B_NORMAL_PRIORITY, this);
    if (fUpdateThread >= 0)
        resume_thread(fUpdateThread);
}

void ProcessView::DetachedFromWindow()
{
    fTerminated = true;
    if (fQuitSem >= 0) {
        delete_sem(fQuitSem);
        fQuitSem = -1;
    }
    if (fUpdateThread != B_ERROR) {
        status_t ret;
        wait_for_thread(fUpdateThread, &ret);
        fUpdateThread = B_ERROR;
    }
}

void ProcessView::MessageReceived(BMessage* message)
{
    switch (message->what) {
        case MSG_KILL_PROCESS:
            KillSelectedProcess();
            break;
        case MSG_SUSPEND_PROCESS:
            SuspendSelectedProcess();
            break;
        case MSG_RESUME_PROCESS:
            ResumeSelectedProcess();
            break;
        case MSG_PRIORITY_LOW:
            SetSelectedProcessPriority(B_LOW_PRIORITY);
            break;
        case MSG_PRIORITY_NORMAL:
            SetSelectedProcessPriority(B_NORMAL_PRIORITY);
            break;
        case MSG_PRIORITY_HIGH:
            SetSelectedProcessPriority(B_URGENT_DISPLAY_PRIORITY);
            break;
        case MSG_PROCESS_DATA_UPDATE:
            Update(message);
            break;
        case MSG_SEARCH_UPDATED:
            FilterRows();
            break;
        case MSG_SHOW_CONTEXT_MENU: {
            BPoint screenWhere;
            if (message->FindPoint("screen_where", &screenWhere) == B_OK) {
                ShowContextMenu(screenWhere);
            }
            break;
        }
        case MSG_CONFIRM_KILL: {
            int32 button_index;
            team_id team;
            if (message->FindInt32("which", &button_index) == B_OK && button_index == 0) {
                if (message->FindInt32("team_id", (int32*)&team) == B_OK) {
                    if (kill_team(team) != B_OK) {
                        BAlert* errAlert = new BAlert(B_TRANSLATE("Error"), B_TRANSLATE("Failed to kill process."), B_TRANSLATE("OK"),
                                                      NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
                        errAlert->Go(NULL); // Asynchronous fire-and-forget
                    }
                }
            }
            break;
        }
        default:
            BView::MessageReceived(message);
            break;
    }
}

void ProcessView::MouseDown(BPoint where)
{
    // Deprecated: Handled by ProcessListView
    BView::MouseDown(where);
}

void ProcessView::KeyDown(const char* bytes, int32 numBytes)
{
    if (numBytes == 1) {
        if (bytes[0] == B_DELETE) {
            KillSelectedProcess();
            return;
        }
    }
    BView::KeyDown(bytes, numBytes);
}

void ProcessView::Hide()
{
    fIsHidden = true;
    BView::Hide();
}

void ProcessView::Show()
{
    fIsHidden = false;
    BView::Show();
}

BString ProcessView::GetUserName(uid_t uid, std::vector<char>& buffer) {
    fCacheLock.Lock();
    auto it = fUserNameCache.find(uid);
    if (it != fUserNameCache.end()) {
        BString name = it->second;
        fCacheLock.Unlock();
        return name;
    }
    fCacheLock.Unlock();

    struct passwd pwd;
    struct passwd* result = NULL;

    BString name;
    if (getpwuid_r(uid, &pwd, buffer.data(), buffer.size(), &result) == 0 && result != NULL) {
        name = result->pw_name;
    } else {
        name << uid;
    }

    fCacheLock.Lock();
    // Re-check in case another thread inserted it
    it = fUserNameCache.find(uid);
    if (it != fUserNameCache.end()) {
        name = it->second;
    } else {
        fUserNameCache.emplace(uid, name);
    }
    fCacheLock.Unlock();
    return name;
}

void ProcessView::ShowContextMenu(BPoint screenPoint) {
    BRow* selectedRow = fProcessListView->CurrentSelection();
    if (!selectedRow) return;

    fContextMenu->SetTargetForItems(this);
    // fProcessListView->ConvertToScreen(&screenPoint); // Argument is already in screen coordinates
    fContextMenu->Go(screenPoint, true, true, true);
}

void ProcessView::KillSelectedProcess() {
    BRow* selectedRow = fProcessListView->CurrentSelection();
    if (!selectedRow) return;

    BIntegerField* pidField = static_cast<BIntegerField*>(selectedRow->GetField(kPIDColumn));
    if (!pidField) return;

    team_id team = pidField->Value();

    BString alertMsg;
    alertMsg.SetToFormat(B_TRANSLATE("Are you sure you want to kill process %d (%s)?"),
                         (int)team,
                         ((BStringField*)selectedRow->GetField(kProcessNameColumn))->String());
    BAlert* confirmAlert = new BAlert(B_TRANSLATE("Confirm Kill"), alertMsg.String(), B_TRANSLATE("Kill"), B_TRANSLATE("Cancel"),
                                      NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);

    BMessage* confirmMsg = new BMessage(MSG_CONFIRM_KILL);
    confirmMsg->AddInt32("team_id", team);
    confirmAlert->Go(new BInvoker(confirmMsg, this));
}

void ProcessView::SuspendSelectedProcess() {
    BRow* selectedRow = fProcessListView->CurrentSelection();
    if (!selectedRow) return;
    BIntegerField* pidField = static_cast<BIntegerField*>(selectedRow->GetField(kPIDColumn));
    if (!pidField) return;
    team_id team = pidField->Value();
    send_signal(team, SIGSTOP);
}

void ProcessView::ResumeSelectedProcess() {
    BRow* selectedRow = fProcessListView->CurrentSelection();
    if (!selectedRow) return;
    BIntegerField* pidField = static_cast<BIntegerField*>(selectedRow->GetField(kPIDColumn));
    if (!pidField) return;
    team_id team = pidField->Value();
    send_signal(team, SIGCONT);
}

void ProcessView::SetSelectedProcessPriority(int32 priority) {
    BRow* selectedRow = fProcessListView->CurrentSelection();
    if (!selectedRow) return;
    BIntegerField* pidField = static_cast<BIntegerField*>(selectedRow->GetField(kPIDColumn));
    if (!pidField) return;
    team_id team = pidField->Value();

    thread_info tInfo;
    int32 cookie = 0;
    while (get_next_thread_info(team, &cookie, &tInfo) == B_OK) {
        set_thread_priority(tInfo.thread, priority);
    }
}

void ProcessView::SetRefreshInterval(bigtime_t interval)
{
    fRefreshInterval = interval;
}

void ProcessView::FilterRows()
{
    const char* searchText = fSearchControl->Text();
    bool filtering = (searchText != NULL && strlen(searchText) > 0);

    for (auto& pair : fTeamRowMap) {
        team_id id = pair.first;
        BRow* row = pair.second;
        bool match = true;

        if (filtering) {
            BStringField* nameField = static_cast<BStringField*>(row->GetField(kProcessNameColumn));
            BString name(nameField->String());
            BString idStr; idStr << id;
            if (name.IFindFirst(searchText) == B_ERROR && idStr.IFindFirst(searchText) == B_ERROR) {
                match = false;
            }
        }

        bool isVisible = fVisibleRows.find(row) != fVisibleRows.end();
        if (match && !isVisible) {
            fProcessListView->AddRow(row);
            fVisibleRows.insert(row);
        } else if (!match && isVisible) {
            fProcessListView->RemoveRow(row);
            fVisibleRows.erase(row);
        }
    }
}

void ProcessView::Update(BMessage* message)
{
    const void* data;
    ssize_t size;
    if (message->FindData("procs", B_RAW_TYPE, &data, &size) != B_OK)
        return;

    const ProcessInfo* infos = (const ProcessInfo*)data;
    size_t count = size / sizeof(ProcessInfo);

    const char* searchText = fSearchControl->Text();
    bool filtering = (searchText != NULL && strlen(searchText) > 0);

    fActiveUIDs.clear();
    fActivePIDs.clear();

    // First pass: Update existing rows or create new ones, and handle visibility
    for (size_t i = 0; i < count; i++) {
        const ProcessInfo& info = infos[i];
        fActiveUIDs.insert(info.userID);
        fActivePIDs.insert(info.id);
        
        // Determine state string
        const char* stateStr = info.state;
        if (strcmp(info.state, "Running") == 0) stateStr = fStrRunning.String();
        else if (strcmp(info.state, "Ready") == 0) stateStr = fStrReady.String();
        else if (strcmp(info.state, "Sleeping") == 0) stateStr = fStrSleeping.String();
        else stateStr = info.state; // Fallback

        BRow* row;
        if (fTeamRowMap.find(info.id) == fTeamRowMap.end()) {
            row = new BRow();
            row->SetField(new BIntegerField(info.id), kPIDColumn);
            row->SetField(new BStringField(info.name), kProcessNameColumn);
            row->SetField(new BStringField(stateStr), kStateColumn);
            row->SetField(new FloatField(info.cpuUsage), kCPUUsageColumn);
            row->SetField(new SizeField(info.memoryUsageBytes), kMemoryUsageColumn);
            row->SetField(new BIntegerField(info.threadCount), kThreadCountColumn);
            row->SetField(new BStringField(info.userName), kUserNameColumn);
            // Don't AddRow here yet, wait for filter check
            fTeamRowMap[info.id] = row;
        } else {
            row = fTeamRowMap[info.id];
            bool changed = false;

            // Helper lambda for updating fields
            auto updateStrField = [&](int index, const char* newVal) {
                BStringField* f = static_cast<BStringField*>(row->GetField(index));
                if (f) {
                    if (strcmp(f->String(), newVal) != 0) {
                        f->SetString(newVal);
                        changed = true;
                    }
                } else {
                    row->SetField(new BStringField(newVal), index);
                    changed = true;
                }
            };

            updateStrField(kProcessNameColumn, info.name);
            updateStrField(kStateColumn, stateStr);

            // CPU
            FloatField* cpuField = static_cast<FloatField*>(row->GetField(kCPUUsageColumn));
            if (cpuField) {
                if (cpuField->Value() != info.cpuUsage) {
                    cpuField->SetValue(info.cpuUsage);
                    changed = true;
                }
            } else {
                row->SetField(new FloatField(info.cpuUsage), kCPUUsageColumn);
                changed = true;
            }

            // Memory
            SizeField* memField = static_cast<SizeField*>(row->GetField(kMemoryUsageColumn));
            if (memField) {
                if (memField->Value() != info.memoryUsageBytes) {
                    memField->SetValue(info.memoryUsageBytes);
                    changed = true;
                }
            } else {
                row->SetField(new SizeField(info.memoryUsageBytes), kMemoryUsageColumn);
                changed = true;
            }

            // Threads
            BIntegerField* threadsField = static_cast<BIntegerField*>(row->GetField(kThreadCountColumn));
            if (threadsField) {
                if (threadsField->Value() != (int32)info.threadCount) {
                    threadsField->SetValue(info.threadCount);
                    changed = true;
                }
            } else {
                row->SetField(new BIntegerField(info.threadCount), kThreadCountColumn);
                changed = true;
            }

            updateStrField(kUserNameColumn, info.userName);

            if (changed && fVisibleRows.find(row) != fVisibleRows.end())
                fProcessListView->UpdateRow(row);
        }

        // Handle filtering
        bool match = true;
        if (filtering) {
            BString name(info.name);
            BString idStr; idStr << info.id;
            if (name.IFindFirst(searchText) == B_ERROR && idStr.IFindFirst(searchText) == B_ERROR) {
                match = false;
            }
        }

        bool isVisible = fVisibleRows.find(row) != fVisibleRows.end();
        if (match && !isVisible) {
            fProcessListView->AddRow(row);
            fVisibleRows.insert(row);
        } else if (!match && isVisible) {
            fProcessListView->RemoveRow(row);
            fVisibleRows.erase(row);
            // Do NOT delete the row, it is still in fTeamRowMap
        }
    }

    // Second pass: Remove dead processes
	for (auto it = fTeamRowMap.begin(); it != fTeamRowMap.end();) {
        if (fActivePIDs.find(it->first) == fActivePIDs.end()) {
			BRow* row = it->second;
            if (fVisibleRows.find(row) != fVisibleRows.end()) {
			    fProcessListView->RemoveRow(row);
                fVisibleRows.erase(row);
            }
			delete row;
			it = fTeamRowMap.erase(it);
		} else {
			++it;
		}
	}

    // Prune user name cache
    BAutolock locker(fCacheLock);
    for (auto it = fUserNameCache.begin(); it != fUserNameCache.end();) {
        if (fActiveUIDs.find(it->first) == fActiveUIDs.end()) {
            it = fUserNameCache.erase(it);
        } else {
            ++it;
        }
    }
}

int32 ProcessView::UpdateThread(void* data)
{
    ProcessView* view = static_cast<ProcessView*>(data);
    BMessenger target(view);

	// Allocated outside the loop to prevent repetitive allocation
	std::unordered_set<thread_id> activeThreads;
    std::vector<ProcessInfo> procList;
    procList.reserve(128);

    // Buffer for getpwuid_r reuse
    long bufSize = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (bufSize == -1) bufSize = 16384;
    std::vector<char> pwdBuffer(bufSize);

    while (!view->fTerminated) {
        if (view->fIsHidden) {
            status_t err = acquire_sem_etc(view->fQuitSem, 1, B_RELATIVE_TIMEOUT, view->fRefreshInterval);
            if (err != B_OK && err != B_TIMED_OUT && err != B_INTERRUPTED) break;
            continue;
        }

		activeThreads.clear();
        procList.clear();

        bigtime_t currentSystemTime = system_time();
        bigtime_t systemTimeDelta = currentSystemTime - view->fLastSystemTime;
        if (systemTimeDelta <= 0) systemTimeDelta = 1;
        view->fLastSystemTime = currentSystemTime;

        system_info sysInfo;
        get_system_info(&sysInfo);
        float totalPossibleCoreTime = sysInfo.cpu_count * systemTimeDelta;
        if (totalPossibleCoreTime <= 0) totalPossibleCoreTime = 1.0f;

        int32 cookie = 0;
        team_info teamInfo;
        while (get_next_team_info(&cookie, &teamInfo) == B_OK) {
            ProcessInfo currentProc;
            currentProc.id = teamInfo.team;

            image_info imgInfo;
            int32 imgCookie = 0;
            if (get_next_image_info(teamInfo.team, &imgCookie, &imgInfo) == B_OK) {
                BPath path(imgInfo.name);
                const char* leafName = NULL;
                if (path.InitCheck() == B_OK)
                    leafName = path.Leaf();

                if (leafName != NULL)
                    strlcpy(currentProc.name, leafName, B_OS_NAME_LENGTH);
                else
                    strlcpy(currentProc.name, imgInfo.name, B_OS_NAME_LENGTH);
            } else {
				strlcpy(currentProc.name, teamInfo.args, B_OS_NAME_LENGTH);
                if (strlen(currentProc.name) == 0)
                    strlcpy(currentProc.name, "system_daemon", B_OS_NAME_LENGTH);
            }

            currentProc.threadCount = teamInfo.thread_count;
            currentProc.areaCount = teamInfo.area_count;
            currentProc.userID = teamInfo.uid;
			BString userName = view->GetUserName(currentProc.userID, pwdBuffer);
			strlcpy(currentProc.userName, userName.String(), B_OS_NAME_LENGTH);

            int32 threadCookie = 0;
            thread_info tInfo;
            bigtime_t teamActiveTimeDelta = 0;

            bool isRunning = false;
            bool isReady = false;

            while (get_next_thread_info(teamInfo.team, &threadCookie, &tInfo) == B_OK) {
				activeThreads.insert(tInfo.thread);
                bigtime_t threadTime = tInfo.user_time + tInfo.kernel_time;

                if (tInfo.state == B_THREAD_RUNNING) isRunning = true;
                if (tInfo.state == B_THREAD_READY) isReady = true;

                if (view->fThreadTimeMap.count(tInfo.thread)) {
                    bigtime_t threadTimeDelta = threadTime - view->fThreadTimeMap[tInfo.thread];
                    if (threadTimeDelta < 0) threadTimeDelta = 0;

                    if (strstr(tInfo.name, "idle thread") == NULL) {
                        teamActiveTimeDelta += threadTimeDelta;
                    }
                }
                view->fThreadTimeMap[tInfo.thread] = threadTime;
            }

            if (isRunning) strlcpy(currentProc.state, "Running", sizeof(currentProc.state));
            else if (isReady) strlcpy(currentProc.state, "Ready", sizeof(currentProc.state));
            else strlcpy(currentProc.state, "Sleeping", sizeof(currentProc.state));

            float teamCpuPercent = (float)teamActiveTimeDelta / totalPossibleCoreTime * 100.0f;
            if (teamCpuPercent < 0.0f) teamCpuPercent = 0.0f;
            if (teamCpuPercent > 100.0f) teamCpuPercent = 100.0f;
            currentProc.cpuUsage = teamCpuPercent;

            currentProc.memoryUsageBytes = 0;
            area_info areaInfo;
            ssize_t areaCookie = 0;
            while (get_next_area_info(teamInfo.team, &areaCookie, &areaInfo) == B_OK) {
                currentProc.memoryUsageBytes += areaInfo.ram_size;
            }

            procList.push_back(currentProc);
        }

		// Prune dead threads from the map
		for (auto it = view->fThreadTimeMap.begin(); it != view->fThreadTimeMap.end();) {
			if (activeThreads.find(it->first) == activeThreads.end())
				it = view->fThreadTimeMap.erase(it);
			else
				++it;
		}

        if (!procList.empty()) {
            BMessage msg(MSG_PROCESS_DATA_UPDATE);
            msg.AddData("procs", B_RAW_TYPE, procList.data(), procList.size() * sizeof(ProcessInfo));
            target.SendMessage(&msg);
        }

        status_t err = acquire_sem_etc(view->fQuitSem, 1, B_RELATIVE_TIMEOUT, view->fRefreshInterval);
        if (err == B_OK) {
            // Drain the semaphore to prevent spinning if multiple updates were requested
            int32 count;
            if (get_sem_count(view->fQuitSem, &count) == B_OK && count > 0)
                acquire_sem_etc(view->fQuitSem, count, B_RELATIVE_TIMEOUT, 0);
        } else if (err != B_TIMED_OUT && err != B_INTERRUPTED) {
            // Semaphore failure (e.g. deleted), exit
            break;
        }
    }

    return B_OK;
}

void ProcessView::SaveState(BMessage& state)
{
    if (fProcessListView) {
        // Save column order
        for (int i = 0; i < fProcessListView->CountColumns(); i++) {
            BColumn* col = fProcessListView->ColumnAt(i);
            const char* id = "unknown";
            if (col == fPIDColumn) id = "pid";
            else if (col == fNameColumn) id = "name";
            else if (col == fStateColumn) id = "state";
            else if (col == fCPUColumn) id = "cpu";
            else if (col == fMemColumn) id = "mem";
            else if (col == fThreadsColumn) id = "threads";
            else if (col == fUserColumn) id = "user";

            state.AddString("col_order", id);

            BString widthKey = "width_"; widthKey << id;
            state.AddFloat(widthKey.String(), col->Width());

            BString visibleKey = "visible_"; visibleKey << id;
            state.AddBool(visibleKey.String(), col->IsVisible());
        }

        // Save sort state
        BColumn* sortCol = NULL;
        bool inverse = false;
        bool sensitive = false;

        // Note: Assuming standard BColumnListView signature
        // If compilation fails here, we will need to omit it or use alternate means
        ((ProcessListView*)fProcessListView)->GetSortColumn(&sortCol, &inverse, &sensitive);

        if (sortCol) {
            const char* id = "unknown";
            if (sortCol == fPIDColumn) id = "pid";
            else if (sortCol == fNameColumn) id = "name";
            else if (sortCol == fStateColumn) id = "state";
            else if (sortCol == fCPUColumn) id = "cpu";
            else if (sortCol == fMemColumn) id = "mem";
            else if (sortCol == fThreadsColumn) id = "threads";
            else if (sortCol == fUserColumn) id = "user";

            state.AddString("sort_col", id);
            state.AddBool("sort_inverse", inverse);
        }
    }
}

void ProcessView::LoadState(const BMessage& state)
{
    if (fProcessListView) {
        // Restore order
        BString id;
        int32 index = 0;
        while (state.FindString("col_order", index, &id) == B_OK) {
            BColumn* col = NULL;
            if (id == "pid") col = fPIDColumn;
            else if (id == "name") col = fNameColumn;
            else if (id == "state") col = fStateColumn;
            else if (id == "cpu") col = fCPUColumn;
            else if (id == "mem") col = fMemColumn;
            else if (id == "threads") col = fThreadsColumn;
            else if (id == "user") col = fUserColumn;

            if (col) {
                // Move column to current index
                fProcessListView->MoveColumn(col, index);

                BString widthKey = "width_"; widthKey << id;
                float width;
                if (state.FindFloat(widthKey.String(), &width) == B_OK)
                    col->SetWidth(width);

                BString visibleKey = "visible_"; visibleKey << id;
                bool visible;
                if (state.FindBool(visibleKey.String(), &visible) == B_OK)
                    col->SetVisible(visible);
            }
            index++;
        }

        // Restore sort
        BString sortId;
        if (state.FindString("sort_col", &sortId) == B_OK) {
            BColumn* col = NULL;
            if (sortId == "pid") col = fPIDColumn;
            else if (sortId == "name") col = fNameColumn;
            else if (sortId == "state") col = fStateColumn;
            else if (sortId == "cpu") col = fCPUColumn;
            else if (sortId == "mem") col = fMemColumn;
            else if (sortId == "threads") col = fThreadsColumn;
            else if (sortId == "user") col = fUserColumn;

            bool inverse = false;
            state.FindBool("sort_inverse", &inverse);

            if (col) {
                fProcessListView->SetSortColumn(col, inverse, true);
            }
        }
    }
}
