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
      fCpuInfos(nullptr),
      fCpuCount(0),
      fPreviousTimeSnapshot(0),
      fCurrentUsage(0.0f),
      fMaxFrequency(0),
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
    // Initialize system info
    system_info sysInfo;
    if (get_system_info(&sysInfo) != B_OK) {
        return;
    }
    fPreviousSysInfo = sysInfo;
    fCpuCount = sysInfo.cpu_count;

    // Header
    BStringView* cpuLabel = new BStringView("cpu_header", "CPU");
    BFont headerFont(be_bold_font);
    headerFont.SetSize(headerFont.Size() * 1.5);
    cpuLabel->SetFont(&headerFont);

    fModelName = new BStringView("model_name", "Processor");
    fModelName->SetAlignment(B_ALIGN_RIGHT);

    // Utilization Header
    BStringView* utilLabel = new BStringView("util_label", B_TRANSLATE("% Utilisation"));
    BStringView* maxUtilLabel = new BStringView("max_util", "100%");
    maxUtilLabel->SetAlignment(B_ALIGN_RIGHT);

    // Core Graphs Grid
    BGridLayout* graphGrid = new BGridLayout(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);

    if (fCpuCount > 0) {
        fPreviousIdleTime = new(std::nothrow) bigtime_t[fCpuCount];
        fCpuInfos = new(std::nothrow) cpu_info[fCpuCount];
        fPerCoreUsage.resize(fCpuCount, 0.0f);

        int cols = ceil(sqrt((double)fCpuCount));
        if (cols < 1) cols = 1;

        for (uint32 i = 0; i < fCpuCount; ++i) {
            ActivityGraphView* graph = new ActivityGraphView("core_graph", {80, 133, 229, 255}, B_NAVIGATION_BASE_COLOR);
            graph->SetExplicitMinSize(BSize(50, 40));
            fCoreGraphs.push_back(graph);

            graphGrid->AddView(graph, i % cols, i / cols);
        }

        if (fPreviousIdleTime && fCpuInfos && get_cpu_info(0, fCpuCount, fCpuInfos) == B_OK) {
            for (uint32 i = 0; i < fCpuCount; ++i) {
                fPreviousIdleTime[i] = fCpuInfos[i].active_time;
            }
        } else if (fPreviousIdleTime) {
            for (uint32 i = 0; i < fCpuCount; ++i) {
                fPreviousIdleTime[i] = 0;
            }
        }
    }

    // Info Grid
    BGridLayout* infoGrid = new BGridLayout(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
    infoGrid->SetInsets(0, B_USE_DEFAULT_SPACING, 0, 0);

    infoGrid->AddView(new BStringView(NULL, B_TRANSLATE("Utilization")), 0, 0);
    infoGrid->AddView(new BStringView(NULL, B_TRANSLATE("Speed")), 1, 0);

    fOverallUsageValue = new BStringView("overall_value", "0.0%");
    BFont bigFont(be_bold_font);
    bigFont.SetSize(bigFont.Size() * 1.2);
    fOverallUsageValue->SetFont(&bigFont);
    infoGrid->AddView(fOverallUsageValue, 0, 1);

    fSpeedValue = new BStringView("speed", "0.00 GHz");
    fSpeedValue->SetFont(&bigFont);
    infoGrid->AddView(fSpeedValue, 1, 1);

    infoGrid->AddView(new BStringView(NULL, B_TRANSLATE("Processes")), 0, 2);
    infoGrid->AddView(new BStringView(NULL, B_TRANSLATE("Threads")), 1, 2);

    fProcessesValue = new BStringView("procs", "0");
    fProcessesValue->SetFont(&bigFont);
    infoGrid->AddView(fProcessesValue, 0, 3);

    fThreadsValue = new BStringView("threads", "0");
    fThreadsValue->SetFont(&bigFont);
    infoGrid->AddView(fThreadsValue, 1, 3);

    infoGrid->AddView(new BStringView(NULL, B_TRANSLATE("Up time")), 1, 4);

    fUptimeValue = new BStringView("uptime", "0:00:00:00");
    fUptimeValue->SetFont(&bigFont);
    infoGrid->AddView(fUptimeValue, 1, 5);

    infoGrid->SetColumnWeight(0, 1.0f);
    infoGrid->SetColumnWeight(1, 1.0f);

    // Initialize Max Frequency once
    cpu_topology_node_info* topology = NULL;
    uint32_t topologyNodeCount = 0;
    if (get_cpu_topology_info(NULL, &topologyNodeCount) == B_OK && topologyNodeCount > 0) {
        topology = new(std::nothrow) cpu_topology_node_info[topologyNodeCount];
        if (topology) {
            uint32_t actualCount = topologyNodeCount;
            if (get_cpu_topology_info(topology, &actualCount) == B_OK) {
                for (uint32 i = 0; i < actualCount; i++) {
                    if (topology[i].type == B_TOPOLOGY_CORE) {
                        if (topology[i].data.core.default_frequency > fMaxFrequency)
                            fMaxFrequency = topology[i].data.core.default_frequency;
                    }
                }
            }
            delete[] topology;
        }
    }

    // Main layout
    BLayoutBuilder::Group<>(this, B_VERTICAL)
        .SetInsets(B_USE_DEFAULT_SPACING)
        .AddGroup(B_HORIZONTAL)
            .Add(cpuLabel)
            .AddGlue()
            .Add(fModelName)
        .End()
        .AddGroup(B_HORIZONTAL)
            .Add(utilLabel)
            .AddGlue()
            .Add(maxUtilLabel)
        .End()
        .Add(graphGrid)
        .Add(infoGrid)
        .AddGlue();
}

CPUView::~CPUView() {
    if (fPreviousIdleTime)
        delete[] fPreviousIdleTime;
    if (fCpuInfos)
        delete[] fCpuInfos;
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
    if (fCpuCount == 0 || fPreviousIdleTime == NULL || fCpuInfos == NULL) {
        overallUsage = -1.0f;
        return;
    }

    bigtime_t currentTimeSnapshot = system_time();
    if (fPreviousTimeSnapshot == 0) {
        fPreviousTimeSnapshot = currentTimeSnapshot;
        // Also update previous idle time to avoid spikes on first update
        if (get_cpu_info(0, fCpuCount, fCpuInfos) == B_OK) {
             for (uint32 i = 0; i < fCpuCount; ++i)
                 fPreviousIdleTime[i] = fCpuInfos[i].active_time;
        }
    }
    bigtime_t elapsedWallTime = currentTimeSnapshot - fPreviousTimeSnapshot;
    fPreviousTimeSnapshot = currentTimeSnapshot;

    if (elapsedWallTime <= 0) {
        overallUsage = 0.0f;
        return;
    }

    float totalDeltaActiveTime = 0;
    if (get_cpu_info(0, fCpuCount, fCpuInfos) == B_OK) {
        for (uint32 i = 0; i < fCpuCount; ++i) {
            bigtime_t delta = fCpuInfos[i].active_time - fPreviousIdleTime[i];
            if (delta < 0) delta = 0; // Handle time rollover

            float coreUsage = (float)delta / elapsedWallTime * 100.0f;
            if (coreUsage < 0.0f) coreUsage = 0.0f;
            if (coreUsage > 100.0f) coreUsage = 100.0f;
            if (i < fPerCoreUsage.size())
                fPerCoreUsage[i] = coreUsage;

            totalDeltaActiveTime += delta;
            fPreviousIdleTime[i] = fCpuInfos[i].active_time;
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

    // Update core graphs
    bigtime_t now = system_time();
    for (size_t i = 0; i < fCoreGraphs.size(); ++i) {
        if (i < fPerCoreUsage.size()) {
            fCoreGraphs[i]->AddValue(now, fPerCoreUsage[i]);
        }
    }

    // Update Info
    system_info sysInfo;
    if (get_system_info(&sysInfo) == B_OK) {
        BString procStr; procStr << sysInfo.used_teams;
        fProcessesValue->SetText(procStr.String());

        BString threadStr; threadStr << sysInfo.used_threads;
        fThreadsValue->SetText(threadStr.String());

        fUptimeValue->SetText(::FormatUptime(system_time()).String());

        // Speed
        if (fMaxFrequency > 0) {
            fSpeedValue->SetText(::FormatHertz(fMaxFrequency).String());
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
