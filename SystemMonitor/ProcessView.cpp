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
#include <set>
#include <Window.h>


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
        char unit1[5] = {0}, unit2[5] = {0};
        sscanf(str1, "%f %s", &val1, unit1);
        sscanf(str2, "%f %s", &val2, unit2);

        if (strcmp(unit1, "MiB") == 0) val1 *= 1024;
        if (strcmp(unit1, "GiB") == 0) val1 *= 1024 * 1024;
        if (strcmp(unit2, "MiB") == 0) val2 *= 1024;
        if (strcmp(unit2, "GiB") == 0) val2 *= 1024 * 1024;

        if (val1 < val2) return -1;
        if (val1 > val2) return 1;
        return 0;
    }
};

enum {
    kPIDColumn,
    kProcessNameColumn,
    kCPUUsageColumn,
    kMemoryUsageColumn,
    kThreadCountColumn,
    kUserNameColumn
};

const uint32 MSG_KILL_PROCESS = 'kill';

ProcessView::ProcessView()
    : BView("ProcessView", B_WILL_DRAW),
      fLastSystemTime(0),
      fUpdateThread(B_ERROR),
      fTerminated(false)
{
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

    fProcessListView = new BColumnListView(Bounds(), "process_clv",
                                           B_FOLLOW_ALL_SIDES,
                                           B_WILL_DRAW | B_NAVIGABLE,
                                           B_PLAIN_BORDER, true);

    fProcessListView->AddColumn(new BIntegerColumn("PID", 60, 30, 100), kPIDColumn);
    fProcessListView->AddColumn(new BStringColumn("Name", 180, 50, 500, B_TRUNCATE_END), kProcessNameColumn);
    fProcessListView->AddColumn(new BCPUColumn("CPU %", 90, 50, 120, B_TRUNCATE_END, B_ALIGN_RIGHT), kCPUUsageColumn);
    fProcessListView->AddColumn(new BMemoryColumn("Memory", 100, 50, 200, B_TRUNCATE_END, B_ALIGN_RIGHT), kMemoryUsageColumn);
    fProcessListView->AddColumn(new BIntegerColumn("Threads", 80, 40, 120, B_ALIGN_RIGHT), kThreadCountColumn);
    fProcessListView->AddColumn(new BStringColumn("User", 80, 40, 150, B_TRUNCATE_END), kUserNameColumn);

    fProcessListView->SetSortColumn(fProcessListView->ColumnAt(kCPUUsageColumn), false, false);

    fContextMenu = new BPopUpMenu("ProcessContext", false, false);
    fContextMenu->AddItem(new BMenuItem("Kill Process", new BMessage(MSG_KILL_PROCESS)));

    BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
        .SetInsets(0)
        .Add(fProcessListView)
    .End();
}

ProcessView::~ProcessView()
{
    fTerminated = true;
    if (fUpdateThread != B_ERROR) {
        status_t ret;
        wait_for_thread(fUpdateThread, &ret);
    }
	for (auto const& [team, row] : fTeamRowMap)
		delete row;
    delete fContextMenu;
}

void ProcessView::AttachedToWindow()
{
    BView::AttachedToWindow();
    fProcessListView->SetTarget(this);
    fLastSystemTime = system_time();

    fUpdateThread = spawn_thread(UpdateThread, "Process Update", B_NORMAL_PRIORITY, this);
    if (fUpdateThread >= 0)
        resume_thread(fUpdateThread);
}

void ProcessView::DetachedFromWindow()
{
    fTerminated = true;
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
        case MSG_PROCESS_DATA_UPDATE:
            Update(message);
            break;
        default:
            BView::MessageReceived(message);
            break;
    }
}

BString ProcessView::GetUserName(uid_t uid) {
    struct passwd* pw = getpwuid(uid);
    if (pw) {
        return BString(pw->pw_name);
    }
    BString idStr;
    idStr << uid;
    return idStr;
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
    alertMsg.SetToFormat("Are you sure you want to kill process %d (%s)?",
                         (int)team,
                         ((BStringField*)selectedRow->GetField(kProcessNameColumn))->String());
    BAlert confirmAlert("Confirm Kill", alertMsg.String(), "Kill", "Cancel",
                                      NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
    int32 button_index = confirmAlert.Go();

    if (button_index == 0) { // "Kill" button
        if (kill_team(team) != B_OK) {
            BAlert errAlert("Error", "Failed to kill process.", "OK",
                                          NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
            errAlert.Go();
        }
    }
}

void ProcessView::Update(BMessage* message)
{
    std::set<team_id> activePIDsThisPulse;
    const ProcessInfo* info;
    ssize_t size;

    for (int i = 0; message->FindData("proc_info", B_RAW_TYPE, i, (const void**)&info, &size) == B_OK; i++) {
        activePIDsThisPulse.insert(info->id);
        
		BRow* row;
		if (fTeamRowMap.find(info->id) == fTeamRowMap.end()) {
            row = new BRow();
            row->SetField(new BIntegerField(info->id), kPIDColumn);
            row->SetField(new BStringField(info->name), kProcessNameColumn);
            char cpuStr[16];
            snprintf(cpuStr, sizeof(cpuStr), "%.1f", info->cpuUsage);
            row->SetField(new BStringField(cpuStr), kCPUUsageColumn);
            row->SetField(new BStringField(::FormatBytes(info->memoryUsageBytes)), kMemoryUsageColumn);
            row->SetField(new BIntegerField(info->threadCount), kThreadCountColumn);
            row->SetField(new BStringField(info->userName), kUserNameColumn);
            fProcessListView->AddRow(row);
			fTeamRowMap[info->id] = row;
        } else { // Existing process
			row = fTeamRowMap[info->id];
			row->SetField(new BStringField(info->name), kProcessNameColumn);
            char cpuStr[16];
            snprintf(cpuStr, sizeof(cpuStr), "%.1f", info->cpuUsage);
			row->SetField(new BStringField(cpuStr), kCPUUsageColumn);
			row->SetField(new BStringField(::FormatBytes(info->memoryUsageBytes)), kMemoryUsageColumn);
			row->SetField(new BIntegerField(info->threadCount), kThreadCountColumn);
			row->SetField(new BStringField(info->userName), kUserNameColumn);
            fProcessListView->UpdateRow(row);
        }
    }

	for (auto it = fTeamRowMap.begin(); it != fTeamRowMap.end();) {
		if (activePIDsThisPulse.find(it->first) == activePIDsThisPulse.end()) {
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
	std::set<thread_id> activeThreads;

    while (!view->fTerminated) {
		activeThreads.clear();
        BMessage msg(MSG_PROCESS_DATA_UPDATE);

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
				strncpy(currentProc.name, path.Leaf(), B_OS_NAME_LENGTH);
				strncpy(currentProc.path, imgInfo.name, B_PATH_NAME_LENGTH);
            } else {
				strncpy(currentProc.name, teamInfo.args, B_OS_NAME_LENGTH);
                if (strlen(currentProc.name) == 0)
                    strncpy(currentProc.name, "system_daemon", B_OS_NAME_LENGTH);
            }

            currentProc.threadCount = teamInfo.thread_count;
            currentProc.areaCount = teamInfo.area_count;
            currentProc.userID = teamInfo.uid;
			BString userName = view->GetUserName(currentProc.userID);
			strncpy(currentProc.userName, userName.String(), B_OS_NAME_LENGTH);

            int32 threadCookie = 0;
            thread_info tInfo;
            bigtime_t teamActiveTimeDelta = 0;

            while (get_next_thread_info(teamInfo.team, &threadCookie, &tInfo) == B_OK) {
				activeThreads.insert(tInfo.thread);
                bigtime_t threadTime = tInfo.user_time + tInfo.kernel_time;
                if (view->fThreadTimeMap.count(tInfo.thread)) {
                    bigtime_t threadTimeDelta = threadTime - view->fThreadTimeMap[tInfo.thread];
                    if (threadTimeDelta < 0) threadTimeDelta = 0;

                    if (strstr(tInfo.name, "idle thread") == NULL) {
                        teamActiveTimeDelta += threadTimeDelta;
                    }
                }
                view->fThreadTimeMap[tInfo.thread] = threadTime;
            }

            float teamCpuPercent = (float)teamActiveTimeDelta / totalPossibleCoreTime * 100.0f;
            if (teamCpuPercent < 0.0f) teamCpuPercent = 0.0f;
            if (teamCpuPercent > 100.0f * sysInfo.cpu_count) teamCpuPercent = 100.0f * sysInfo.cpu_count;
            currentProc.cpuUsage = teamCpuPercent;

            currentProc.memoryUsageBytes = 0;
            area_info areaInfo;
            ssize_t areaCookie = 0;
            while (get_next_area_info(teamInfo.team, &areaCookie, &areaInfo) == B_OK) {
                currentProc.memoryUsageBytes += areaInfo.ram_size;
            }

            msg.AddData("proc_info", B_RAW_TYPE, &currentProc, sizeof(ProcessInfo));
        }

		// Prune dead threads from the map
		for (auto it = view->fThreadTimeMap.begin(); it != view->fThreadTimeMap.end();) {
			if (activeThreads.find(it->first) == activeThreads.end())
				it = view->fThreadTimeMap.erase(it);
			else
				++it;
		}

        if (view->Window())
            view->Window()->PostMessage(&msg, view);

        snooze(1000000);
    }

    return B_OK;
}
