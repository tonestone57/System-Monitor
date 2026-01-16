#include "CPUView.h"
#include <Autolock.h>
#include <cstdio>
#include <String.h>
#include <kernel/OS.h>
#include <LayoutBuilder.h>
#include <string.h>
#include <Box.h>
#include <GridLayout.h>
#include <SpaceLayoutItem.h>
#include <InterfaceDefs.h>
#include <Catalog.h>
#include "Utils.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "CPUView"

CPUView::CPUView()
    : BView("CPUView", B_WILL_DRAW | B_PULSE_NEEDED),
      fPreviousIdleTime(nullptr),
      fCpuCount(0),
      fPreviousTimeSnapshot(0),
      fCurrentUsage(0.0f),
      fSpeedValue(NULL),
      fProcessesValue(NULL),
      fThreadsValue(NULL),
      fUptimeValue(NULL)
{
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    CreateLayout();
}

void CPUView::CreateLayout()
{
    // Create graph view
    fGraphView = new ActivityGraphView("cpu_graph", {0, 150, 0, 255}, B_SUCCESS_COLOR);
    fGraphView->SetExplicitMinSize(BSize(200, 150));

    // Info Grid
    BGridLayout* infoGrid = new BGridLayout(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
    infoGrid->SetInsets(0, B_USE_DEFAULT_SPACING, 0, 0);

    // Row 0 Labels
    infoGrid->AddView(new BStringView(NULL, B_TRANSLATE("Utilization")), 0, 0);
    infoGrid->AddView(new BStringView(NULL, B_TRANSLATE("Speed")), 1, 0);
    
    // Row 1 Values
    fOverallUsageValue = new BStringView("overall_value", "0.0%");
    BFont bigFont(be_bold_font);
    bigFont.SetSize(bigFont.Size() * 1.2);
    fOverallUsageValue->SetFont(&bigFont);
    infoGrid->AddView(fOverallUsageValue, 0, 1);

    fSpeedValue = new BStringView("speed", "0.00 GHz");
    fSpeedValue->SetFont(&bigFont);
    infoGrid->AddView(fSpeedValue, 1, 1);

    // Row 2 Labels
    infoGrid->AddView(new BStringView(NULL, B_TRANSLATE("Processes")), 0, 2);
    infoGrid->AddView(new BStringView(NULL, B_TRANSLATE("Threads")), 1, 2);

    // Row 3 Values
    fProcessesValue = new BStringView("procs", "0");
    fProcessesValue->SetFont(&bigFont);
    infoGrid->AddView(fProcessesValue, 0, 3);

    fThreadsValue = new BStringView("threads", "0");
    fThreadsValue->SetFont(&bigFont);
    infoGrid->AddView(fThreadsValue, 1, 3);

    // Row 4 Labels
    infoGrid->AddView(new BStringView(NULL, B_TRANSLATE("Up time")), 1, 4);

    // Row 5 Values
    fUptimeValue = new BStringView("uptime", "0:00:00:00");
    fUptimeValue->SetFont(&bigFont);
    infoGrid->AddView(fUptimeValue, 1, 5);

    infoGrid->SetColumnWeight(0, 1.0f);
    infoGrid->SetColumnWeight(1, 1.0f);

    // Main layout
    BLayoutBuilder::Group<>(this, B_VERTICAL)
        .SetInsets(B_USE_DEFAULT_SPACING)
        .Add(fGraphView)
        .Add(infoGrid)
        .AddGlue();

    // Initialize system info
    system_info sysInfo;
    if (get_system_info(&sysInfo) != B_OK) {
        fOverallUsageValue->SetText(B_TRANSLATE("Error fetching CPU info"));
        return;
    }

    fPreviousSysInfo = sysInfo;
    fCpuCount = sysInfo.cpu_count;

    if (fCpuCount > 0) {
        fPreviousIdleTime = new(std::nothrow) bigtime_t[fCpuCount];
        if (!fPreviousIdleTime) {
            fOverallUsageValue->SetText(B_TRANSLATE("Memory allocation failed"));
            return;
        }
        for (uint32 i = 0; i < fCpuCount; ++i) {
            cpu_info info;
            if (get_cpu_info(i, 1, &info) == B_OK)
                fPreviousIdleTime[i] = info.active_time;
            else
                fPreviousIdleTime[i] = 0;
        }
    } else {
        fOverallUsageValue->SetText(B_TRANSLATE("No CPU data"));
    }
}

CPUView::~CPUView() {
    if (fPreviousIdleTime)
        delete[] fPreviousIdleTime;
}

void CPUView::AttachedToWindow() {
    SetFlags(Flags() | B_PULSE_NEEDED);
    if (Window())
        Window()->SetPulseRate(1000000); // 1 second
    UpdateData(); // Initial data fetch
    BView::AttachedToWindow();
}

void CPUView::Pulse() {
    UpdateData();
}

void CPUView::GetCPUUsage(float& overallUsage)
{
    if (fCpuCount == 0 || fPreviousIdleTime == NULL) {
        overallUsage = -1.0f;
        return;
    }

    bigtime_t currentTimeSnapshot = system_time();
    if (fPreviousTimeSnapshot == 0)
        fPreviousTimeSnapshot = currentTimeSnapshot;
    bigtime_t elapsedWallTime = currentTimeSnapshot - fPreviousTimeSnapshot;
    fPreviousTimeSnapshot = currentTimeSnapshot;

    if (elapsedWallTime <= 0) {
        overallUsage = 0.0f;
        return;
    }

    float totalDeltaActiveTime = 0;

    for (uint32 i = 0; i < fCpuCount; ++i) {
        cpu_info info;
        if (get_cpu_info(i, 1, &info) == B_OK) {
            bigtime_t delta = info.active_time - fPreviousIdleTime[i];
            if (delta < 0) delta = 0; // Handle time rollover
            totalDeltaActiveTime += delta;
            fPreviousIdleTime[i] = info.active_time;
        }
    }

    overallUsage = (float)totalDeltaActiveTime / (elapsedWallTime * fCpuCount) * 100.0f;

    if (overallUsage < 0.0f) overallUsage = 0.0f;
    if (overallUsage > 100.0f) overallUsage = 100.0f;
}

void CPUView::UpdateData()
{
    fLocker.Lock();

    float overallUsage;
    GetCPUUsage(overallUsage);

    if (overallUsage >= 0) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%.1f%%", overallUsage);
        fOverallUsageValue->SetText(buffer);
    } else {
        fOverallUsageValue->SetText("N/A");
    }

    if (fGraphView && overallUsage >= 0)
        fGraphView->AddValue(system_time(), overallUsage);

    // Update Info
    system_info sysInfo;
    if (get_system_info(&sysInfo) == B_OK) {
        BString procStr; procStr << sysInfo.used_teams;
        fProcessesValue->SetText(procStr.String());

        BString threadStr; threadStr << sysInfo.used_threads;
        fThreadsValue->SetText(threadStr.String());

        fUptimeValue->SetText(::FormatUptime(system_time()).String());

        // Speed
        cpu_topology_node_info* topology = NULL;
        uint32_t topologyNodeCount = 0;
        if (get_cpu_topology_info(NULL, &topologyNodeCount) == B_OK && topologyNodeCount > 0) {
             topology = new(std::nothrow) cpu_topology_node_info[topologyNodeCount];
             if (topology) {
                 uint32_t actualCount = topologyNodeCount;
                 if (get_cpu_topology_info(topology, &actualCount) == B_OK) {
                     uint64 maxFreq = 0;
                     for (uint32 i = 0; i < actualCount; i++) {
                         if (topology[i].type == B_TOPOLOGY_CORE) {
                             if (topology[i].data.core.default_frequency > maxFreq)
                                 maxFreq = topology[i].data.core.default_frequency;
                         }
                     }
                     if (maxFreq > 0)
                         fSpeedValue->SetText(::FormatHertz(maxFreq).String());
                 }
                 delete[] topology;
             }
        }
    }

    fCurrentUsage = overallUsage;
    fLocker.Unlock();
}

float CPUView::GetCurrentUsage()
{
	BAutolock locker(fLocker);
    return fCurrentUsage;
}

void CPUView::Draw(BRect updateRect) {
    BView::Draw(updateRect);
}
