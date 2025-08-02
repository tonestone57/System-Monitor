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

CPUView::CPUView()
    : BView("CPUView", B_WILL_DRAW | B_PULSE_NEEDED),
      fPreviousIdleTime(nullptr),
      fCpuCount(0),
      fPreviousTimeSnapshot(0),
      fCurrentUsage(0.0f)
{
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    CreateLayout();
}

void CPUView::CreateLayout()
{
    // Create overall usage box
    BBox* overallBox = new BBox("OverallCPUBox");
    overallBox->SetLabel("Overall CPU Usage");

    // Create grid layout for CPU stats
    BGridLayout* grid = new BGridLayout(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
    grid->SetInsets(B_USE_DEFAULT_SPACING);

    // Add CPU usage display
    BStringView* overallLabel = new BStringView("overall_label", "Total Usage:");
    fOverallUsageValue = new BStringView("overall_value", "0.0%");
    
    grid->AddView(overallLabel, 0, 0);
    grid->AddView(fOverallUsageValue, 1, 0);
    grid->AddItem(BSpaceLayoutItem::CreateGlue(), 2, 0);
    grid->SetColumnWeight(2, 1.0f);
    
    overallBox->SetLayout(grid);

    // Create graph view
    fGraphView = new ActivityGraphView("cpu_graph", {0, 150, 0, 255});
    fGraphView->SetExplicitMinSize(BSize(200, 80));

    // Main layout
    BLayoutBuilder::Group<>(this, B_VERTICAL)
        .SetInsets(B_USE_DEFAULT_SPACING)
        .Add(overallBox)
        .Add(fGraphView)
        .AddGlue();

    // Initialize system info
    system_info sysInfo;
    if (get_system_info(&sysInfo) != B_OK) {
        fOverallUsageValue->SetText("Error fetching CPU info");
        return;
    }

    fPreviousSysInfo = sysInfo;
    fCpuCount = sysInfo.cpu_count;

    if (fCpuCount > 0) {
        fPreviousIdleTime = new(std::nothrow) bigtime_t[fCpuCount];
        if (!fPreviousIdleTime) {
            fOverallUsageValue->SetText("Memory allocation failed");
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
        fOverallUsageValue->SetText("No CPU data");
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