#include "ProcessView.h"
#include "Utils.h"
#include <LayoutBuilder.h>
#include <ListView.h>
#include <ListItem.h>
#include <StringView.h>
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
#include <ScrollView.h>
#include <Autolock.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ProcessView"

// Define constants for columns (used for drawing)
const float kPIDWidth = 60;
const float kNameWidth = 180;
const float kStateWidth = 80;
const float kCPUWidth = 60;
const float kMemWidth = 90;
const float kThreadsWidth = 60;
const float kUserWidth = 80;

namespace {

class ProcessListItem : public BListItem {
public:
    ProcessListItem(const ProcessInfo& info, const char* stateStr, const BFont* font)
        : BListItem()
    {
        Update(info, stateStr, font);
    }

    void Update(const ProcessInfo& info, const char* stateStr, const BFont* font) {
        fInfo = info;

        // Cache display strings
        fCachedPID.SetToFormat("%" B_PRId32, fInfo.id);
        fCachedName = fInfo.name;
        fCachedState = stateStr;
        fCachedCPU.SetToFormat("%.1f", fInfo.cpuUsage);
        fCachedMem = FormatBytes(fInfo.memoryUsageBytes);
        fCachedThreads.SetToFormat("%" B_PRIu32, fInfo.threadCount);
        fCachedUser = fInfo.userName;

        // Truncate strings if font is provided
        if (font) {
            font->TruncateString(&fCachedName, B_TRUNCATE_END, kNameWidth - 10, &fTruncatedName);
            font->TruncateString(&fCachedUser, B_TRUNCATE_END, kUserWidth - 10, &fTruncatedUser);
        } else {
            fTruncatedName = fCachedName;
            fTruncatedUser = fCachedUser;
        }
    }

    virtual void DrawItem(BView* owner, BRect itemRect, bool complete = false) {
        if (IsSelected() || complete) {
            rgb_color color;
            if (IsSelected()) color = ui_color(B_LIST_SELECTED_BACKGROUND_COLOR);
            else color = ui_color(B_LIST_BACKGROUND_COLOR);

            owner->SetHighColor(color);
            owner->FillRect(itemRect);
        }

        rgb_color textColor;
        if (IsSelected()) textColor = ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR);
        else textColor = ui_color(B_LIST_ITEM_TEXT_COLOR);
        owner->SetHighColor(textColor);

        font_height fh;
        owner->GetFont(&fh);
        float x = itemRect.left + 5;
        float y = itemRect.bottom - fh.descent;

        // PID
        owner->DrawString(fCachedPID.String(), BPoint(x, y));
        x += kPIDWidth;

        // Name
        owner->DrawString(fTruncatedName.String(), BPoint(x, y));
        x += kNameWidth;

        // State
        owner->DrawString(fCachedState.String(), BPoint(x, y));
        x += kStateWidth;

        // CPU
        owner->DrawString(fCachedCPU.String(), BPoint(x, y));
        x += kCPUWidth;

        // Mem
        owner->DrawString(fCachedMem.String(), BPoint(x, y));
        x += kMemWidth;

        // Threads
        owner->DrawString(fCachedThreads.String(), BPoint(x, y));
        x += kThreadsWidth;

        // User
        owner->DrawString(fTruncatedUser.String(), BPoint(x, y));
    }

    team_id TeamID() const { return fInfo.id; }
    const char* Name() const { return fInfo.name; }

    // For searching
    const ProcessInfo& Info() const { return fInfo; }

    static int CompareCPU(const void* first, const void* second) {
        const ProcessListItem* item1 = *(const ProcessListItem**)first;
        const ProcessListItem* item2 = *(const ProcessListItem**)second;
        if (item1->fInfo.cpuUsage > item2->fInfo.cpuUsage) return -1;
        if (item1->fInfo.cpuUsage < item2->fInfo.cpuUsage) return 1;
        return 0;
    }

private:
    ProcessInfo fInfo;
    BString fCachedPID;
    BString fCachedName;
    BString fCachedState;
    BString fCachedCPU;
    BString fCachedMem;
    BString fCachedThreads;
    BString fCachedUser;

    BString fTruncatedName;
    BString fTruncatedUser;
};

class ProcessListView : public BListView {
public:
    ProcessListView(const char* name)
        : BListView(name, B_SINGLE_SELECTION_LIST, B_WILL_DRAW | B_NAVIGABLE)
    {
    }

    virtual void MouseDown(BPoint where) {
        BMessage* msg = Window()->CurrentMessage();
        int32 buttons = 0;
        msg->FindInt32("buttons", &buttons);

        if (buttons & B_SECONDARY_MOUSE_BUTTON) {
            Select(IndexOf(where)); // Select item under mouse

            BMessenger messenger(Target());
            if (messenger.IsValid()) {
                BMessage contextMsg('cntx'); // MSG_SHOW_CONTEXT_MENU
                BPoint screenWhere = where;
                ConvertToScreen(&screenWhere);
                contextMsg.AddPoint("screen_where", screenWhere);
                messenger.SendMessage(&contextMsg);
            }
        }
        BListView::MouseDown(where);
    }

    virtual void KeyDown(const char* bytes, int32 numBytes) {
        if (numBytes == 1 && bytes[0] == B_DELETE) {
            BMessenger messenger(Target());
            if (messenger.IsValid())
                messenger.SendMessage('kill'); // MSG_KILL_PROCESS
            return;
        }
        BListView::KeyDown(bytes, numBytes);
    }
};

} // namespace


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
      fFilterName(""),
      fFilterID(""),
      fUpdateThread(B_ERROR),
      fTerminated(false),
      fIsHidden(false)
{
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

    fQuitSem = create_sem(0, "ProcessView Quit");

    fSearchControl = new BTextControl("Search", B_TRANSLATE("Search:"), "", new BMessage(MSG_SEARCH_UPDATED));
    fSearchControl->SetModificationMessage(new BMessage(MSG_SEARCH_UPDATED));

    fProcessListView = new ProcessListView("process_list");
    BScrollView* processScrollView = new BScrollView("process_scroll", fProcessListView, 0, false, true, true);

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

    // Header View construction
    BGroupView* headerView = new BGroupView(B_HORIZONTAL, 0);
    headerView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

    // Helper to add header label
    auto addHeader = [&](const char* label, float width) {
        BStringView* sv = new BStringView(NULL, label);
        sv->SetExplicitMinSize(BSize(width, B_SIZE_UNSET));
        sv->SetExplicitMaxSize(BSize(width, B_SIZE_UNSET));
        sv->SetAlignment(B_ALIGN_LEFT);
        sv->SetFont(be_bold_font);
        headerView->AddChild(sv);
    };

    addHeader(B_TRANSLATE("PID"), kPIDWidth);
    addHeader(B_TRANSLATE("Name"), kNameWidth);
    addHeader(B_TRANSLATE("State"), kStateWidth);
    addHeader(B_TRANSLATE("CPU%"), kCPUWidth);
    addHeader(B_TRANSLATE("Mem"), kMemWidth);
    addHeader(B_TRANSLATE("Thds"), kThreadsWidth);
    addHeader(B_TRANSLATE("User"), kUserWidth);

    headerView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 20));

    BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
        .SetInsets(0)
        .Add(fSearchControl)
        .Add(headerView)
        .Add(processScrollView)
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

    fProcessListView->MakeEmpty(); // Just clears pointers
    fVisibleItems.clear();
    for (auto& pair : fTeamItemMap) {
        delete pair.second;
    }
    fTeamItemMap.clear();
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
                        errAlert->Go(NULL);
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
    int32 selection = fProcessListView->CurrentSelection();
    if (selection < 0) return;

    fContextMenu->SetTargetForItems(this);
    fContextMenu->Go(screenPoint, true, true, true);
}

void ProcessView::KillSelectedProcess() {
    int32 selection = fProcessListView->CurrentSelection();
    if (selection < 0) return;

    ProcessListItem* item = dynamic_cast<ProcessListItem*>(fProcessListView->ItemAt(selection));
    if (!item) return;

    team_id team = item->TeamID();

    BString alertMsg;
    alertMsg.SetToFormat(B_TRANSLATE("Are you sure you want to kill process %d (%s)?"),
                         (int)team, item->Name());
    BAlert* confirmAlert = new BAlert(B_TRANSLATE("Confirm Kill"), alertMsg.String(), B_TRANSLATE("Kill"), B_TRANSLATE("Cancel"),
                                      NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);

    BMessage* confirmMsg = new BMessage(MSG_CONFIRM_KILL);
    confirmMsg->AddInt32("team_id", team);
    confirmAlert->Go(new BInvoker(confirmMsg, this));
}

void ProcessView::SuspendSelectedProcess() {
    int32 selection = fProcessListView->CurrentSelection();
    if (selection < 0) return;
    ProcessListItem* item = dynamic_cast<ProcessListItem*>(fProcessListView->ItemAt(selection));
    if (!item) return;
    send_signal(item->TeamID(), SIGSTOP);
}

void ProcessView::ResumeSelectedProcess() {
    int32 selection = fProcessListView->CurrentSelection();
    if (selection < 0) return;
    ProcessListItem* item = dynamic_cast<ProcessListItem*>(fProcessListView->ItemAt(selection));
    if (!item) return;
    send_signal(item->TeamID(), SIGCONT);
}

void ProcessView::SetSelectedProcessPriority(int32 priority) {
    int32 selection = fProcessListView->CurrentSelection();
    if (selection < 0) return;
    ProcessListItem* item = dynamic_cast<ProcessListItem*>(fProcessListView->ItemAt(selection));
    if (!item) return;

    team_id team = item->TeamID();
    thread_info tInfo;
    int32 cookie = 0;
    while (get_next_thread_info(team, &cookie, &tInfo) == B_OK) {
        set_thread_priority(tInfo.thread, priority);
    }
}

void ProcessView::SetRefreshInterval(bigtime_t interval)
{
    fRefreshInterval = interval;
    if (fQuitSem >= 0)
        release_sem(fQuitSem);
}

void ProcessView::FilterRows()
{
    const char* searchText = fSearchControl->Text();
    bool filtering = (searchText != NULL && strlen(searchText) > 0);

    // BListView doesn't support hiding items easily.
    // We have to remove them from the list but keep them in fTeamItemMap.

    fProcessListView->MakeEmpty(); // Clear visualization (pointers only)
    fVisibleItems.clear();

    for (auto& pair : fTeamItemMap) {
        team_id id = pair.first;
        ProcessListItem* item = pair.second;
        bool match = true;

        if (filtering) {
            fFilterName.SetTo(item->Info().name);
            fFilterID.SetToFormat("%" B_PRId32, id);

            if (fFilterName.IFindFirst(searchText) == B_ERROR && fFilterID.IFindFirst(searchText) == B_ERROR) {
                match = false;
            }
        }

        if (match) {
            fProcessListView->AddItem(item);
            fVisibleItems.insert(item);
        }
    }

    // Restore selection? Too hard for now.
    fProcessListView->SortItems(ProcessListItem::CompareCPU);
    fProcessListView->Invalidate();
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

    bool listChanged = false;

    // Get font once
    BFont font;
    fProcessListView->GetFont(&font);

    // First pass: Update existing items or create new ones
    for (size_t i = 0; i < count; i++) {
        const ProcessInfo& info = infos[i];
        fActiveUIDs.insert(info.userID);
        fActivePIDs.insert(info.id);
        
        const char* stateStr = info.state;
        if (strcmp(info.state, "Running") == 0) stateStr = fStrRunning.String();
        else if (strcmp(info.state, "Ready") == 0) stateStr = fStrReady.String();
        else if (strcmp(info.state, "Sleeping") == 0) stateStr = fStrSleeping.String();
        else stateStr = info.state;

        ProcessListItem* item;
        if (fTeamItemMap.find(info.id) == fTeamItemMap.end()) {
            item = new ProcessListItem(info, stateStr, &font);
            fTeamItemMap[info.id] = item;

            // Check filter before adding
            bool match = true;
            if (filtering) {
                 fFilterName.SetTo(info.name);
                 fFilterID.SetToFormat("%" B_PRId32, info.id);
                 if (fFilterName.IFindFirst(searchText) == B_ERROR && fFilterID.IFindFirst(searchText) == B_ERROR) {
                     match = false;
                 }
            }
            if (match) {
                fProcessListView->AddItem(item);
                fVisibleItems.insert(item);
                listChanged = true;
            }
        } else {
            item = fTeamItemMap[info.id];
            item->Update(info, stateStr, &font);
        }
    }

    // Second pass: Remove dead processes
	for (auto it = fTeamItemMap.begin(); it != fTeamItemMap.end();) {
        if (fActivePIDs.find(it->first) == fActivePIDs.end()) {
			ProcessListItem* item = it->second;
            if (fVisibleItems.find(item) != fVisibleItems.end()) {
                fProcessListView->RemoveItem(item);
                fVisibleItems.erase(item);
                listChanged = true;
            }
			delete item;
			it = fTeamItemMap.erase(it);
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

    fProcessListView->SortItems(ProcessListItem::CompareCPU);
    fProcessListView->Invalidate();
}

int32 ProcessView::UpdateThread(void* data)
{
    // Implementation remains largely same, just logic to fetch data
    ProcessView* view = static_cast<ProcessView*>(data);
    BMessenger target(view);

	std::unordered_set<thread_id> activeThreads;
    std::vector<ProcessInfo> procList;
    procList.reserve(128);

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
            int32 count;
            if (get_sem_count(view->fQuitSem, &count) == B_OK && count > 0)
                acquire_sem_etc(view->fQuitSem, count, B_RELATIVE_TIMEOUT, 0);
        } else if (err != B_TIMED_OUT && err != B_INTERRUPTED) {
            break;
        }
    }

    return B_OK;
}

void ProcessView::SaveState(BMessage& state)
{
    // Not implemented for BListView version yet
}

void ProcessView::LoadState(const BMessage& state)
{
    // Not implemented for BListView version yet
}
