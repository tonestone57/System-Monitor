#include "ProcessView.h"
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
#include <stdio.h>
#include <MenuItem.h>
#include <Font.h>
#include <set>


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
        // Simple comparison based on string length first, then lexicographically
        // This is a heuristic and might not be perfect for all cases (e.g. 900 KiB vs 1.0 MiB)
        // A more robust solution would parse the units (KiB, MiB) and convert to a common base.
        const char* str1 = ((BStringField*)field1)->String();
        const char* str2 = ((BStringField*)field2)->String();

        // For the scope of this fix, we'll assume sorting by magnitude is the goal
        // and we can achieve that by parsing the value.
        // A more complex implementation would be needed for perfect sorting of formatted strings.
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


// Column identifiers
enum {
    kPIDColumn,
    kProcessNameColumn,
    kCPUUsageColumn,
    kMemoryUsageColumn,
    kThreadCountColumn,
    kUserNameColumn
};

// Context Menu Messages
const uint32 MSG_KILL_PROCESS = 'kill';
const uint32 MSG_SORT_COLUMN = 'sort';

ProcessView::ProcessView(BRect frame)
    : BView(frame, "ProcessView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_PULSE_NEEDED),
      fLastPulseSystemTime(0)
{
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

    fProcessListView = new BColumnListView(Bounds(), "process_clv",
                                           B_FOLLOW_ALL_SIDES,
                                           B_WILL_DRAW | B_NAVIGABLE,
                                           B_PLAIN_BORDER, true);

    fProcessListView->AddColumn(new BIntegerColumn("PID", 60, 30, 100), kPIDColumn);
    fProcessListView->AddColumn(new BStringColumn("Name", 180, 50, 500, B_TRUNCATE_END), kProcessNameColumn);
    fProcessListView->AddColumn(new BCPUColumn("CPU %", 70, 40, 100, B_TRUNCATE_END, B_ALIGN_RIGHT), kCPUUsageColumn);
    fProcessListView->AddColumn(new BMemoryColumn("Memory", 100, 50, 200, B_TRUNCATE_END, B_ALIGN_RIGHT), kMemoryUsageColumn);
    fProcessListView->AddColumn(new BIntegerColumn("Threads", 60, 30, 100, B_ALIGN_RIGHT), kThreadCountColumn);
    fProcessListView->AddColumn(new BStringColumn("User", 80, 40, 150, B_TRUNCATE_END), kUserNameColumn);

    fProcessListView->SetSortColumn(fProcessListView->ColumnAt(kCPUUsageColumn), false, false);

    // Context Menu
    fContextMenu = new BPopUpMenu("ProcessContext", false, false);
    fContextMenu->AddItem(new BMenuItem("Kill Process", new BMessage(MSG_KILL_PROCESS)));

    BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
        .SetInsets(0)
        .Add(fProcessListView)
    .End();
}

ProcessView::~ProcessView()
{
    delete fContextMenu;
}

void ProcessView::AttachedToWindow()
{
    fProcessListView->SetTarget(this);
    // for (int32 i = 0; i < fProcessListView->CountColumns(); i++) {
    //     BColumn* column = fProcessListView->ColumnAt(i);
    //     BMessage* message = new BMessage(MSG_SORT_COLUMN);
    //     message->AddInt32("column", i);
    //     column->SetInvoker(new BInvoker(message, this));
    // }
    UpdateData();
    fLastPulseSystemTime = system_time();
    BView::AttachedToWindow();
}

void ProcessView::MessageReceived(BMessage* message)
{
    switch (message->what) {
        case MSG_KILL_PROCESS:
            KillSelectedProcess();
            break;
        // case MSG_SORT_COLUMN:
        // {
        //     int32 column_index;
        //     if (message->FindInt32("column", &column_index) == B_OK) {
        //         fProcessListView->SetSortColumn(fProcessListView->ColumnAt(column_index),
        //             !fProcessListView->ColumnAt(column_index)->IsSortKey(), false);
        //         fProcessListView->Invalidate();
        //     }
        //     break;
        // }
        default:
            BView::MessageReceived(message);
            break;
    }
}

void ProcessView::Pulse()
{
    UpdateData();
}

BString ProcessView::FormatBytes(uint64 bytes) {
    BString str;
    double kb = bytes / 1024.0;
    double mb = kb / 1024.0;

    if (mb >= 1.0) {
        str.SetToFormat("%.1f MiB", mb);
    } else if (kb >= 1.0) {
        str.SetToFormat("%.0f KiB", kb);
    } else {
        str.SetToFormat("%llu B", (unsigned long long)bytes);
    }
    return str;
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

    BIntegerField* pidField = dynamic_cast<BIntegerField*>(selectedRow->GetField(kPIDColumn));
    if (!pidField) return;

    team_id team = pidField->Value();

    BString alertMsg;
    alertMsg.SetToFormat("Are you sure you want to kill process %d (%s)?",
                         (int)team,
                         ((BStringField*)selectedRow->GetField(kProcessNameColumn))->String());
    BAlert* confirmAlert = new BAlert("Confirm Kill", alertMsg.String(), "Kill", "Cancel",
                                      NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
    int32 button_index = confirmAlert->Go();

    if (button_index == 0) { // "Kill" button
        if (kill_team(team) != B_OK) {
            BAlert* errAlert = new BAlert("Error", "Failed to kill process.", "OK",
                                          NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
            errAlert->Go();
        } else {
            fProcessListView->RemoveRow(selectedRow);
            delete selectedRow;
            fProcessTimeMap.erase(team);
        }
    }
}

void ProcessView::UpdateData()
{
    fLocker.Lock();

    bigtime_t currentSystemTime = system_time();
    bigtime_t systemTimeDelta = currentSystemTime - fLastPulseSystemTime;
    if (systemTimeDelta <= 0) systemTimeDelta = 1;

    system_info sysInfo;
    get_system_info(&sysInfo);
    float totalPossibleCoreTime = sysInfo.cpu_count * systemTimeDelta;
    if (totalPossibleCoreTime <= 0) totalPossibleCoreTime = 1.0f;

    std::set<team_id> activePIDsThisPulse;

    int32 cookie = 0;
    team_info teamInfo;
    while (get_next_team_info(&cookie, &teamInfo) == B_OK) {
        activePIDsThisPulse.insert(teamInfo.team);
        ProcessInfo currentProc;
        currentProc.id = teamInfo.team;
        
        // Get process name from image info
        image_info imgInfo;
        int32 imgCookie = 0;
        if (get_next_image_info(teamInfo.team, &imgCookie, &imgInfo) == B_OK) {
            BPath path(imgInfo.name);
            currentProc.name = path.Leaf();
            currentProc.path = imgInfo.name;
        } else {
            currentProc.name = teamInfo.args;
            if (currentProc.name.Length() == 0) 
                currentProc.name = "system_daemon";
        }

        currentProc.threadCount = teamInfo.thread_count;
        currentProc.areaCount = teamInfo.area_count;
        currentProc.userID = teamInfo.uid;
        currentProc.userName = GetUserName(currentProc.userID);
        
        // Calculate CPU time
        uint64_t procTotalUserTime = 0;
        uint64_t procTotalKernelTime = 0;
        int32 threadCookie = 0;
        thread_info tInfo;

        while (get_next_thread_info(teamInfo.team, &threadCookie, &tInfo) == B_OK) {
            procTotalUserTime += tInfo.user_time;
            procTotalKernelTime += tInfo.kernel_time;
        }

        currentProc.totalUserTime = procTotalUserTime;
        currentProc.totalKernelTime = procTotalKernelTime;
        
        // Calculate Memory Usage
        currentProc.memoryUsageBytes = 0;
        area_info areaInfo;
        ssize_t areaCookie = 0;
        while (get_next_area_info(teamInfo.team, &areaCookie, &areaInfo) == B_OK) {
            currentProc.memoryUsageBytes += areaInfo.ram_size;
        }

        // Calculate CPU Usage
        float cpuPercent = 0.0f;
        if (fProcessTimeMap.count(currentProc.id)) {
            ProcessInfo& prevProc = fProcessTimeMap[currentProc.id];
            bigtime_t procKernelDelta = currentProc.totalKernelTime - prevProc.totalKernelTime;
            bigtime_t procUserDelta = currentProc.totalUserTime - prevProc.totalUserTime;
            bigtime_t procTotalTimeDelta = procKernelDelta + procUserDelta;

            if (procTotalTimeDelta < 0) procTotalTimeDelta = 0;

            cpuPercent = (float)procTotalTimeDelta / totalPossibleCoreTime * 100.0f;
            if (cpuPercent < 0.0f) cpuPercent = 0.0f;
            if (cpuPercent > 100.0f) cpuPercent = 100.0f;
        }
        currentProc.cpuUsage = cpuPercent;

        // Store current times for next calculation
        fProcessTimeMap[currentProc.id] = currentProc;

        // Find or Create Row in BColumnListView
        BRow* row = NULL;
        for (int32 i = 0; (row = fProcessListView->RowAt(i)) != NULL; i++) {
            BIntegerField* pidField = dynamic_cast<BIntegerField*>(row->GetField(kPIDColumn));
            if (pidField && pidField->Value() == currentProc.id) {
                break;
            }
        }

        if (row == NULL) { // New process
            row = new BRow();
            row->SetField(new BIntegerField(currentProc.id), kPIDColumn);
            row->SetField(new BStringField(currentProc.name.String()), kProcessNameColumn);

            char cpuStr[16];
            snprintf(cpuStr, sizeof(cpuStr), "%.1f", currentProc.cpuUsage);
            row->SetField(new BStringField(cpuStr), kCPUUsageColumn);

            row->SetField(new BStringField(FormatBytes(currentProc.memoryUsageBytes).String()), kMemoryUsageColumn);
            row->SetField(new BIntegerField(currentProc.threadCount), kThreadCountColumn);
            row->SetField(new BStringField(currentProc.userName.String()), kUserNameColumn);
            fProcessListView->AddRow(row);
        } else { // Existing process, update fields
            ((BStringField*)row->GetField(kProcessNameColumn))->SetString(currentProc.name.String());

            char cpuStr[16];
            snprintf(cpuStr, sizeof(cpuStr), "%.1f", currentProc.cpuUsage);
            ((BStringField*)row->GetField(kCPUUsageColumn))->SetString(cpuStr);

            ((BStringField*)row->GetField(kMemoryUsageColumn))->SetString(FormatBytes(currentProc.memoryUsageBytes).String());
            ((BIntegerField*)row->GetField(kThreadCountColumn))->SetValue(currentProc.threadCount);
            ((BStringField*)row->GetField(kUserNameColumn))->SetString(currentProc.userName.String());
            fProcessListView->UpdateRow(row);
        }
    }

    // Remove Rows for Processes That No Longer Exist
    for (int32 i = 0; i < fProcessListView->CountRows(); ) {
        BRow* row = fProcessListView->RowAt(i);
        BIntegerField* pidField = dynamic_cast<BIntegerField*>(row->GetField(kPIDColumn));
        if (pidField) {
            if (activePIDsThisPulse.find(pidField->Value()) == activePIDsThisPulse.end()) {
                fProcessTimeMap.erase(pidField->Value());
                fProcessListView->RemoveRow(row);
                delete row;
                continue;
            }
        }
        i++;
    }

    fLastPulseSystemTime = currentSystemTime;
    fLocker.Unlock();
}
