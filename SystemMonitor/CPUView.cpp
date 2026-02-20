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
	  fSpeedValue(NULL),
	  fProcessesValue(NULL),
	  fThreadsValue(NULL),
	  fUptimeValue(NULL),
	  fCpuCount(0),
	  fPreviousTimeSnapshot(0),
	  fCurrentUsage(0.0f),
	  fLastUsedTeams(-1),
	  fLastUsedThreads(-1)
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
	BStringView* cpuLabel = new BStringView("cpu_header", B_TRANSLATE("CPU"));
	BFont headerFont(be_bold_font);
	headerFont.SetSize(headerFont.Size() * 1.5);
	cpuLabel->SetFont(&headerFont);

	fModelName = new BStringView("model_name", GetCPUBrandString().String());
	fModelName->SetAlignment(B_ALIGN_RIGHT);

	// Utilization Header
	BStringView* utilLabel = new BStringView("util_label", B_TRANSLATE("% Utilisation"));
	BStringView* maxUtilLabel = new BStringView("max_util", "100%");
	maxUtilLabel->SetAlignment(B_ALIGN_RIGHT);

	// Core Graphs Grid
	BGridLayout* graphGrid = new BGridLayout(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);

	if (fCpuCount > 0) {
		fPreviousActiveTime.resize(fCpuCount, 0);
		fCpuInfos.resize(fCpuCount);
		fPerCoreUsage.resize(fCpuCount, 0.0f);

		int cols = static_cast<int>(ceil(sqrt(static_cast<double>(fCpuCount))));
		if (cols < 1) cols = 1;

		for (uint32 i = 0; i < fCpuCount; ++i) {
			ActivityGraphView* graph = new ActivityGraphView("core_graph", {80, 133, 229, 255}, B_NAVIGATION_BASE_COLOR);
			graph->SetExplicitMinSize(BSize(50, 40));
			graph->SetManualScale(0, 1000);
			fCoreGraphs.push_back(graph);

			graphGrid->AddView(graph, i % cols, i / cols);
		}

		if (get_cpu_info(0, fCpuCount, fCpuInfos.data()) == B_OK) {
			for (uint32 i = 0; i < fCpuCount; ++i) {
				fPreviousActiveTime[i] = fCpuInfos[i].active_time;
			}
		} else {
			for (uint32 i = 0; i < fCpuCount; ++i) {
				fPreviousActiveTime[i] = 0;
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

	// Get CPU Speed (static)
	uint64 frequency = GetCpuFrequency();
	if (frequency > 0)
		fSpeedValue->SetText(::FormatHertz(frequency).String());
	else
		fSpeedValue->SetText(B_TRANSLATE("N/A"));

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
}

void CPUView::AttachedToWindow() {
	SetFlags(Flags() | B_PULSE_NEEDED);
	UpdateData(); // Initial data fetch
	BView::AttachedToWindow();
}

void CPUView::Pulse() {
	if (IsHidden())
		return;
	UpdateData();
}

void CPUView::GetCPUUsage(bigtime_t now, float& overallUsage)
{
	if (fCpuCount == 0 || fPreviousActiveTime.empty() || fCpuInfos.empty()) {
		overallUsage = -1.0f;
		return;
	}

	bigtime_t currentTimeSnapshot = now;
	if (fPreviousTimeSnapshot == 0) {
		fPreviousTimeSnapshot = currentTimeSnapshot;
		// Also update previous active time to avoid spikes on first update
		if (get_cpu_info(0, fCpuCount, fCpuInfos.data()) == B_OK) {
			 for (uint32 i = 0; i < fCpuCount; ++i)
				 fPreviousActiveTime[i] = fCpuInfos[i].active_time;
		}
	}
	bigtime_t elapsedWallTime = currentTimeSnapshot - fPreviousTimeSnapshot;
	fPreviousTimeSnapshot = currentTimeSnapshot;

	if (elapsedWallTime <= 0) {
		overallUsage = 0.0f;
		return;
	}

	float totalDeltaActiveTime = 0;
	if (get_cpu_info(0, fCpuCount, fCpuInfos.data()) == B_OK) {
		for (uint32 i = 0; i < fCpuCount; ++i) {
			bigtime_t delta = fCpuInfos[i].active_time - fPreviousActiveTime[i];
			if (delta < 0) delta = 0; // Handle time rollover

			float coreUsage = static_cast<float>(delta) / elapsedWallTime * 100.0f;
			if (coreUsage < 0.0f) coreUsage = 0.0f;
			if (coreUsage > 100.0f) coreUsage = 100.0f;
			if (i < fPerCoreUsage.size())
				fPerCoreUsage[i] = coreUsage;

			totalDeltaActiveTime += delta;
			fPreviousActiveTime[i] = fCpuInfos[i].active_time;
		}
	}

	overallUsage = static_cast<float>(totalDeltaActiveTime) / (static_cast<float>(elapsedWallTime) * fCpuCount) * 100.0f;

	if (overallUsage < 0.0f) overallUsage = 0.0f;
	if (overallUsage > 100.0f) overallUsage = 100.0f;
}

void CPUView::UpdateData()
{
	fLocker.Lock();
	const bigtime_t now = system_time();

	float overallUsage;
	GetCPUUsage(now, overallUsage);

	if (fOverallUsageValue) {
		if (overallUsage >= 0) {
			BString percentStr;
			fNumberFormat.FormatPercent(percentStr, overallUsage / 100.0f);
			fOverallUsageValue->SetText(percentStr.String());
		} else {
			fOverallUsageValue->SetText(B_TRANSLATE("N/A"));
		}
	}

	// Update core graphs
	for (size_t i = 0; i < fCoreGraphs.size(); ++i) {
		if (i < fPerCoreUsage.size()) {
			fCoreGraphs[i]->AddValue(now, fPerCoreUsage[i] * 10);
		}
	}

	// Update Info
	system_info sysInfo;
	if (get_system_info(&sysInfo) == B_OK) {
		if (fProcessesValue && (int32)sysInfo.used_teams != fLastUsedTeams) {
			fLastUsedTeams = sysInfo.used_teams;
			fCachedProcesses.SetToFormat("%" B_PRId32, fLastUsedTeams);
			fProcessesValue->SetText(fCachedProcesses.String());
		}

		if (fThreadsValue && (int32)sysInfo.used_threads != fLastUsedThreads) {
			fLastUsedThreads = sysInfo.used_threads;
			fCachedThreads.SetToFormat("%" B_PRId32, fLastUsedThreads);
			fThreadsValue->SetText(fCachedThreads.String());
		}

		if (fUptimeValue)
			fUptimeValue->SetText(::FormatUptime(now).String());
	}

	fCurrentUsage = overallUsage;
	fLocker.Unlock();
}

float CPUView::GetCurrentUsage()
{
	BAutolock locker(fLocker);
	return fCurrentUsage;
}

void CPUView::SetRefreshInterval(bigtime_t interval)
{
	BAutolock locker(fLocker);
	for (auto* graph : fCoreGraphs) {
		if (graph)
			graph->SetRefreshInterval(interval);
	}
}

void CPUView::Draw(BRect updateRect) {
	BView::Draw(updateRect);
}
