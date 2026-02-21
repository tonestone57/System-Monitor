#include "PerformanceView.h"

#include <TabView.h>
#include <LayoutBuilder.h>
#include <Box.h>
#include <StringView.h>
#include <Font.h>
#include <Catalog.h>

#include "CPUView.h"
#include "MemView.h"
#include "NetworkView.h"
#include "DiskView.h"
#include "GPUView.h"
#include "ActivityGraphView.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PerformanceView"


// ---------------------------------------------------------------------------
// SummaryView - small left-panel with CPU/Mem/Net overview graphs
// ---------------------------------------------------------------------------

class SummaryView : public BView {
public:
	SummaryView(SystemStats* stats)
		: BView("SummaryView", B_WILL_DRAW), fStats(stats)
	{
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

		fCpuGraph = new ActivityGraphView("cpu_summary_graph",
			{0, 0, 0, 0}, B_SUCCESS_COLOR);
		fCpuGraph->SetExplicitMinSize(BSize(B_SIZE_UNSET, 60));
		fCpuGraph->SetManualScale(0, 1000);

		fMemGraph = new ActivityGraphView("mem_summary_graph",
			{0, 0, 0, 0}, B_MENU_SELECTION_BACKGROUND_COLOR);
		fMemGraph->SetExplicitMinSize(BSize(B_SIZE_UNSET, 60));
		fMemGraph->SetManualScale(0, 1000);

		fNetGraph = new ActivityGraphView("net_summary_graph",
			{0, 0, 0, 0}, B_FAILURE_COLOR);
		fNetGraph->SetExplicitMinSize(BSize(B_SIZE_UNSET, 60));

		BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
			.SetInsets(B_USE_DEFAULT_SPACING)
			.Add(_CreateCard(B_TRANSLATE("CPU"), fCpuGraph))
			.Add(_CreateCard(B_TRANSLATE("Memory"), fMemGraph))
			.Add(_CreateCard(B_TRANSLATE("Network"), fNetGraph))
			.AddGlue();
	}

	void SetRefreshInterval(bigtime_t interval) {
		if (fCpuGraph) fCpuGraph->SetRefreshInterval(interval);
		if (fMemGraph) fMemGraph->SetRefreshInterval(interval);
		if (fNetGraph) fNetGraph->SetRefreshInterval(interval);
	}

	void UpdateData() {
		if (fStats) {
			const bigtime_t now = system_time();
			fCpuGraph->AddValue(now, fStats->cpuUsage * 10);
			fMemGraph->AddValue(now, fStats->memoryUsage * 10);
			fNetGraph->AddValue(now, fStats->uploadSpeed + fStats->downloadSpeed);
		}
	}

private:
	BView* _CreateCard(const char* label, BView* content) {
		BBox* card = new BBox(B_FANCY_BORDER, NULL);
		BStringView* labelView = new BStringView(NULL, label);
		BFont font(be_bold_font);
		labelView->SetFont(&font);

		BLayoutBuilder::Group<>(card, B_VERTICAL, 0)
			.SetInsets(B_USE_DEFAULT_SPACING / 2)
			.Add(labelView)
			.AddStrut(5)
			.Add(content);
		return card;
	}

	ActivityGraphView*	fCpuGraph;
	ActivityGraphView*	fMemGraph;
	ActivityGraphView*	fNetGraph;
	SystemStats*		fStats;
};


// ---------------------------------------------------------------------------
// PerformanceView
// ---------------------------------------------------------------------------

PerformanceView::PerformanceView()
	: BView("PerformanceView", B_WILL_DRAW | B_PULSE_NEEDED)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BSplitView* splitView = new BSplitView(B_HORIZONTAL, B_USE_DEFAULT_SPACING);
	splitView->SetInsets(B_USE_DEFAULT_SPACING);

	fSummaryView = new SummaryView(&fStats);

	BTabView* tabView = new BTabView("tab_view", B_WIDTH_FROM_WIDEST);
	fRightPane = tabView;

	fCPUView     = new CPUView();
	fMemView     = new MemView();
	fNetworkView = new NetworkView();
	fDiskView    = new DiskView();
	fGPUView     = new GPUView();

	tabView->AddTab(fCPUView);
	tabView->TabAt(0)->SetLabel(B_TRANSLATE("CPU"));
	tabView->AddTab(fMemView);
	tabView->TabAt(1)->SetLabel(B_TRANSLATE("Memory"));
	tabView->AddTab(fNetworkView);
	tabView->TabAt(2)->SetLabel(B_TRANSLATE("Network"));
	tabView->AddTab(fDiskView);
	tabView->TabAt(3)->SetLabel(B_TRANSLATE("Disk"));
	tabView->AddTab(fGPUView);
	tabView->TabAt(4)->SetLabel(B_TRANSLATE("GPU"));

	splitView->AddChild(fSummaryView);
	splitView->AddChild(fRightPane);
	splitView->SetItemWeight(0, 0.25f, false);

	fSplitView = splitView;

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(splitView)
		.End();
}


void
PerformanceView::AttachedToWindow()
{
	BView::AttachedToWindow();
}


void
PerformanceView::Pulse()
{
	if (IsHidden()) return;

	if (!fCPUView->IsHidden()) fCPUView->UpdateData();
	if (!fMemView->IsHidden()) fMemView->UpdateData();

	fStats.cpuUsage      = fCPUView->GetCurrentUsage();
	fStats.memoryUsage   = fMemView->GetCurrentUsage();
	fStats.uploadSpeed   = fNetworkView->GetUploadSpeed();
	fStats.downloadSpeed = fNetworkView->GetDownloadSpeed();

	if (fSummaryView)
		fSummaryView->UpdateData();
}


void
PerformanceView::Hide()
{
	BView::Hide();
	fNetworkView->SetPerformanceViewVisible(false);
	fDiskView->SetPerformanceViewVisible(false);
}


void
PerformanceView::Show()
{
	BView::Show();
	fNetworkView->SetPerformanceViewVisible(true);
	fDiskView->SetPerformanceViewVisible(true);
}


void
PerformanceView::SetRefreshInterval(bigtime_t interval)
{
	if (fSummaryView)  fSummaryView->SetRefreshInterval(interval);
	if (fCPUView)      fCPUView->SetRefreshInterval(interval);
	if (fMemView)      fMemView->SetRefreshInterval(interval);
	if (fNetworkView)  fNetworkView->SetRefreshInterval(interval);
	if (fDiskView)     fDiskView->SetRefreshInterval(interval);
	if (fGPUView)      fGPUView->SetRefreshInterval(interval);
}


void
PerformanceView::SaveState(BMessage& state)
{
	if (fSplitView) {
		float weight = fSplitView->ItemWeight(0);
		state.AddFloat("perf_split_weight", weight);
	}
}


void
PerformanceView::LoadState(const BMessage& state)
{
	float weight;
	if (fSplitView && state.FindFloat("perf_split_weight", &weight) == B_OK)
		fSplitView->SetItemWeight(0, weight, false);
}
