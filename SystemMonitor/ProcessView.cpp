#include "ProcessView.h"
#include "Utils.h"
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
#include <Messenger.h>
#include <Catalog.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ProcessView"


namespace {

class BCPUColumn : public BStringColumn {
public:
    BCPUColumn(const char* title, float width, float minWidth, float maxWidth,
                uint32 truncate, alignment align = B_ALIGN_LEFT)
        : BStringColumn(title, width, minWidth, maxWidth, truncate, align) {}

    virtual int CompareFields(BField* field1, BField* field2) {
        float val1 = atof(((BStringField*)field1)->String());
        float val2 = atof(((BStringField*)field2)->String());
        if (val1 < val2) return -1;
        if (val1 > val2) return 1;
        return 0;
    }
};

class BMemoryColumn : public BStringColumn {
public:
    BMemoryColumn(const char* title, float width, float minWidth, float maxWidth,
                   uint32 truncate, alignment align = B_ALIGN_LEFT)
        : BStringColumn(title, width, minWidth, maxWidth, truncate, align) {}

    virtual int CompareFields(BField* field1, BField* field2) {
        const char* str1 = ((BStringField*)field1)->String();
        const char* str2 = ((BStringField*)field2)->String();
        float val1 = 0, val2 = 0;
        char unit1[16] = {0}, unit2[16] = {0};
        sscanf(str1, "%f %15s", &val1, unit1);
        sscanf(str2, "%f %15s", &val2, unit2);

        if (strcmp(unit1, "MiB") == 0) val1 *= 1024;
        if (strcmp(unit1, "GiB") == 0) val1 *= 1024 * 1024;
        if (strcmp(unit2, "MiB") == 0) val2 *= 1024;
        if (strcmp(unit2, "GiB") == 0) val2 *= 1024 * 1024;

        if (val1 < val2) return -1;
        if (val1 > val2) return 1;
        return 0;
    }
};

class ProcessListView : public BColumnListView {
public:
    ProcessListView(BRect rect, const char* name, uint32 resizingMode, uint32 flags,
        border_style border = B_NO_BORDER, bool showHorizontalScrollbar = true)
        : BColumnListView(rect, name, resizingMode, flags, border, showHorizontalScrollbar),
          fSortColumn(NULL),
          fSortInverse(false)
    {
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

ProcessView::ProcessView()
    : BView("ProcessView", B_WILL_DRAW),
      fLastSystemTime(0),
      fRefreshInterval(1000000),
      fUpdateThread(B_ERROR),
      fTerminated(false)
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

    fCPUColumn = new BCPUColumn(B_TRANSLATE("Total CPU %"), 90, 50, 120, B_TRUNCATE_END, B_ALIGN_RIGHT);
    fProcessListView->AddColumn(fCPUColumn, kCPUUsageColumn);

    fMemColumn = new BMemoryColumn(B_TRANSLATE("Memory"), 100, 50, 200, B_TRUNCATE_END, B_ALIGN_RIGHT);
    fProcessListView->AddColumn(fMemColumn, kMemoryUsageColumn);

    fThreadsColumn = new BIntegerColumn(B_TRANSLATE("Threads"), 80, 40, 120, B_ALIGN_RIGHT);
    fProcessListView->AddColumn(fThreadsColumn, kThreadCountColumn);

    fStateColumn = new BStringColumn(B_TRANSLATE("State"), 80, 40, 150, B_TRUNCATE_END);
    fProcessListView->AddColumn(fStateColumn, kStateColumn);

    fUserColumn = new BStringColumn(B_TRANSLATE("User"), 80, 40, 150, B_TRUNCATE_END);
    fProcessListView->AddColumn(fUserColumn, kUserNameColumn);

    fProcessListView->SetSortColumn(fCPUColumn, false, false);

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
}

void ProcessView::AttachedToWindow()
{
    BView::AttachedToWindow();
    fProcessListView->SetTarget(this);
    fSearchControl->SetTarget(this);
    fLastSystemTime = system_time();

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
            // Release the semaphore to wake up the thread immediately
            // This triggers an immediate refresh cycle
            if (fQuitSem >= 0)
                release_sem(fQuitSem);
            break;
        default:
            BView::MessageReceived(message);
            break;
    }
}

const BString& ProcessView::GetUserName(uid_t uid) {
    auto it = fUserNameCache.find(uid);
    if (it != fUserNameCache.end()) {
        return it->second;
    }

    struct passwd pwd;
    struct passwd* result = NULL;
    long bufSize = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (bufSize == -1) bufSize = 16384;

    std::vector<char> buffer(bufSize);

    BString name;
    if (getpwuid_r(uid, &pwd, buffer.data(), buffer.size(), &result) == 0 && result != NULL) {
        name = result->pw_name;
    } else {
        name << uid;
    }

    return fUserNameCache.emplace(uid, name).first->second;
}

void ProcessView::ShowContextMenu(BPoint screenPoint) {
    BRow* selectedRow = fProcessListView->CurrentSelection();
    if (!selectedRow) return;

    fContextMenu->SetTargetForItems(this);
    fProcessListView->ConvertToScreen(&screenPoint);
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
    BAlert confirmAlert(B_TRANSLATE("Confirm Kill"), alertMsg.String(), B_TRANSLATE("Kill"), B_TRANSLATE("Cancel"),
                                      NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
    int32 button_index = confirmAlert.Go();

    if (button_index == 0) { // "Kill" button
        if (kill_team(team) != B_OK) {
            BAlert errAlert(B_TRANSLATE("Error"), B_TRANSLATE("Failed to kill process."), B_TRANSLATE("OK"),
                                          NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
            errAlert.Go();
        }
    }
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

void ProcessView::Update(BMessage* message)
{
    std::unordered_set<team_id> activePIDsThisPulse;

    const void* data;
    ssize_t size;
    if (message->FindData("procs", B_RAW_TYPE, &data, &size) != B_OK)
        return;

    const ProcessInfo* infos = (const ProcessInfo*)data;
    size_t count = size / sizeof(ProcessInfo);

    const char* searchText = fSearchControl->Text();
    bool filtering = (searchText != NULL && strlen(searchText) > 0);

    for (size_t i = 0; i < count; i++) {
        const ProcessInfo& info = infos[i];
        
        bool match = true;
        if (filtering) {
            BString name(info.name);
            BString idStr; idStr << info.id;
            if (name.IFindFirst(searchText) == B_ERROR && idStr.IFindFirst(searchText) == B_ERROR) {
                match = false;
            }
        }

        if (match)
            activePIDsThisPulse.insert(info.id);

        if (match) {
            BRow* row;
            if (fTeamRowMap.find(info.id) == fTeamRowMap.end()) {
                row = new BRow();
                row->SetField(new BIntegerField(info.id), kPIDColumn);
                row->SetField(new BStringField(info.name), kProcessNameColumn);
                row->SetField(new BStringField(B_TRANSLATE(info.state)), kStateColumn);
                char cpuStr[16];
                snprintf(cpuStr, sizeof(cpuStr), "%.1f", info.cpuUsage);
                row->SetField(new BStringField(cpuStr), kCPUUsageColumn);
                row->SetField(new BStringField(::FormatBytes(info.memoryUsageBytes)), kMemoryUsageColumn);
                row->SetField(new BIntegerField(info.threadCount), kThreadCountColumn);
                row->SetField(new BStringField(info.userName), kUserNameColumn);
                fProcessListView->AddRow(row);
                fTeamRowMap[info.id] = row;
            } else { // Existing process
                row = fTeamRowMap[info.id];
                bool changed = false;

                BStringField* nameField = static_cast<BStringField*>(row->GetField(kProcessNameColumn));
                if (nameField) {
                     if (strcmp(nameField->String(), info.name) != 0) {
                        nameField->SetString(info.name);
                        changed = true;
                     }
                } else {
                    row->SetField(new BStringField(info.name), kProcessNameColumn);
                    changed = true;
                }

                BStringField* stateField = static_cast<BStringField*>(row->GetField(kStateColumn));
                BString stateStr = B_TRANSLATE(info.state);
                if (stateField) {
                     if (strcmp(stateField->String(), stateStr.String()) != 0) {
                        stateField->SetString(stateStr);
                        changed = true;
                     }
                } else {
                    row->SetField(new BStringField(stateStr), kStateColumn);
                    changed = true;
                }

                char cpuStr[16];
                snprintf(cpuStr, sizeof(cpuStr), "%.1f", info.cpuUsage);
                BStringField* cpuField = static_cast<BStringField*>(row->GetField(kCPUUsageColumn));
                if (cpuField) {
                    if (strcmp(cpuField->String(), cpuStr) != 0) {
                        cpuField->SetString(cpuStr);
                        changed = true;
                     }
                } else {
                    row->SetField(new BStringField(cpuStr), kCPUUsageColumn);
                    changed = true;
                }

                BString memStr = ::FormatBytes(info.memoryUsageBytes);
                BStringField* memField = static_cast<BStringField*>(row->GetField(kMemoryUsageColumn));
                if (memField) {
                    if (strcmp(memField->String(), memStr.String()) != 0) {
                        memField->SetString(memStr);
                        changed = true;
                     }
                } else {
                    row->SetField(new BStringField(memStr), kMemoryUsageColumn);
                    changed = true;
                }

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

                BStringField* userField = static_cast<BStringField*>(row->GetField(kUserNameColumn));
                if (userField) {
                    if (strcmp(userField->String(), info.userName) != 0) {
                        userField->SetString(info.userName);
                        changed = true;
                     }
                } else {
                    row->SetField(new BStringField(info.userName), kUserNameColumn);
                    changed = true;
                }

                if (changed)
                    fProcessListView->UpdateRow(row);
            }
        }
    }

	for (auto it = fTeamRowMap.begin(); it != fTeamRowMap.end();) {
        bool presentInPulse = (activePIDsThisPulse.find(it->first) != activePIDsThisPulse.end());

        if (!presentInPulse) {
			BRow* row = it->second;
			fProcessListView->RemoveRow(row);

			delete row;
			it = fTeamRowMap.erase(it);
		} else {
			++it;
		}
	}
}

int32 ProcessView::UpdateThread(void* data)
{
    ProcessView* view = static_cast<ProcessView*>(data);
	std::unordered_set<thread_id> activeThreads;
    BMessenger target(view);

    while (!view->fTerminated) {
		activeThreads.clear();
        std::vector<ProcessInfo> procList;

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
				strlcpy(currentProc.name, path.Leaf(), B_OS_NAME_LENGTH);
				strlcpy(currentProc.path, imgInfo.name, B_PATH_NAME_LENGTH);
            } else {
				strlcpy(currentProc.name, teamInfo.args, B_OS_NAME_LENGTH);
                if (strlen(currentProc.name) == 0)
                    strlcpy(currentProc.name, "system_daemon", B_OS_NAME_LENGTH);
            }

            currentProc.threadCount = teamInfo.thread_count;
            currentProc.areaCount = teamInfo.area_count;
            currentProc.userID = teamInfo.uid;
			const BString& userName = view->GetUserName(currentProc.userID);
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

        if (acquire_sem_etc(view->fQuitSem, 1, B_RELATIVE_TIMEOUT, view->fRefreshInterval) == B_OK) {
            // Drain the semaphore to prevent spinning if multiple updates were requested
            int32 count;
            if (get_sem_count(view->fQuitSem, &count) == B_OK && count > 0)
                acquire_sem_etc(view->fQuitSem, count, B_RELATIVE_TIMEOUT, 0);
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
