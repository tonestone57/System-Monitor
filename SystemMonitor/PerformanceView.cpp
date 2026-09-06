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
#include "Utils.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PerformanceView"


// ---------------------------------------------------------------------------
// SummaryView - small left-panel with CPU/Mem/Net overview graphs
// ---------------------------------------------------------------------------

class SummaryView : public BView {
public:
	SummaryView(SystemStats* stats)
		: BView("SummaryView", B_WILL_DRAW | B_SUPPORTS_LAYOUT),
		  fCpuInfoText(NULL), fMemInfoText(NULL), fNetInfoText(NULL), fStats(stats)
	{
		SetViewColor(ui_color(B_DOCUMENT_BACKGROUND_COLOR));

		fCpuGraph = new ActivityGraphView("cpu_summary_graph",
			make_color(17, 124, 214, 255), (color_which)-1);
		fCpuGraph->SetFillColor(make_color(195, 236, 250, 255));
		fCpuGraph->SetDrawGrid(false);
		fCpuGraph->SetViewColor(ui_color(B_DOCUMENT_BACKGROUND_COLOR));
		fCpuGraph->SetExplicitMinSize(BSize(60, 60));
		fCpuGraph->SetManualScale(0, 1000);

		fMemGraph = new ActivityGraphView("mem_summary_graph",
			make_color(9, 91, 222, 255), (color_which)-1);
		fMemGraph->SetFillColor(make_color(201, 225, 255, 255));
		fMemGraph->SetDrawGrid(false);
		fMemGraph->SetViewColor(ui_color(B_DOCUMENT_BACKGROUND_COLOR));
		fMemGraph->SetExplicitMinSize(BSize(60, 60));
		fMemGraph->SetManualScale(0, 1000);

		fNetGraph = new ActivityGraphView("net_summary_graph",
			make_color(191, 23, 79, 255), (color_which)-1);
		fNetGraph->SetFillColor(make_color(251, 211, 222, 255));
		fNetGraph->SetDrawGrid(false);
		fNetGraph->SetViewColor(ui_color(B_DOCUMENT_BACKGROUND_COLOR));
		fNetGraph->SetExplicitMinSize(BSize(60, 60));

		SetExplicitMinSize(BSize(150, B_SIZE_UNSET));
		BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
			.SetInsets(B_USE_DEFAULT_SPACING)
			.Add(_CreateCard(B_TRANSLATE("CPU"), fCpuGraph, &fCpuInfoText))
			.Add(_CreateCard(B_TRANSLATE("Memory"), fMemGraph, &fMemInfoText))
			.Add(_CreateCard(B_TRANSLATE("Network"), fNetGraph, &fNetInfoText))
			.AddGlue()
			.End();

		SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
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

			if (fCpuInfoText) {
				BString cpuStr;
				if (fStats->cpuFrequency > 0) {
					BString freqStr = ::FormatHertz(fStats->cpuFrequency);
					cpuStr.SetToFormat(B_TRANSLATE("%.0f%% %s"), fStats->cpuUsage, freqStr.String());
				} else {
					cpuStr.SetToFormat("%.0f%%", fStats->cpuUsage);
				}
				fCpuInfoText->SetText(cpuStr.String());
			}

			if (fMemInfoText) {
				BString memStr;
				uint64 usedBytes = fStats->memoryUsed;
				BString usedStr, totalStr;
				::FormatBytes(usedStr, usedBytes);
				::FormatBytes(totalStr, fStats->memoryTotal);
				memStr.SetToFormat(B_TRANSLATE("%s / %s (%.0f%%)"), usedStr.String(), totalStr.String(), fStats->memoryUsage);
				fMemInfoText->SetText(memStr.String());
			}

			if (fNetInfoText) {
				BString txStr = FormatSpeed(static_cast<uint64>(fStats->uploadSpeed), 1000000);
				BString rxStr = FormatSpeed(static_cast<uint64>(fStats->downloadSpeed), 1000000);
				BString netStr;
				netStr.SetToFormat(B_TRANSLATE("S: %s R: %s"), txStr.String(), rxStr.String());
				fNetInfoText->SetText(netStr.String());
			}
		}
	}

private:
	BView* _CreateCard(const char* label, BView* content, BStringView** infoTextOut) {
		BView* card = new BView(NULL, B_WILL_DRAW | B_SUPPORTS_LAYOUT);
		card->SetViewColor(ui_color(B_DOCUMENT_BACKGROUND_COLOR));

		BBox* borderBox = new BBox("border");
		borderBox->SetFlags(borderBox->Flags() | B_SUPPORTS_LAYOUT);
		borderBox->SetExplicitMinSize(BSize(62, 62));
		borderBox->SetBorder(B_PLAIN_BORDER);
		BLayoutBuilder::Group<>(borderBox, B_HORIZONTAL, 0)
			.SetInsets(1)
			.Add(content)
			.End();

		BStringView* labelView = new BStringView(NULL, label);
		labelView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
		labelView->SetHighColor(ui_color(B_DOCUMENT_TEXT_COLOR)); // Force explicitly
		// Reverted the font.SetSize(+2) to default be_bold_font just like original, to see if font resize broke it!
		BFont font(be_bold_font);
		labelView->SetFont(&font);

		BStringView* infoText = new BStringView(NULL, " ");
		infoText->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
		infoText->SetHighColor(tint_color(ui_color(B_DOCUMENT_TEXT_COLOR), B_LIGHTEN_2_TINT)); // explicit grey
		*infoTextOut = infoText;

		BLayoutBuilder::Group<>(card, B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.SetInsets(B_USE_DEFAULT_SPACING / 2)
			.Add(borderBox)
			.AddGroup(B_VERTICAL, 0)
				.Add(labelView)
				.Add(infoText)
				.AddGlue()
			.End()
			.End();

		card->SetExplicitMinSize(BSize(62, 62));
		card->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
		return card;
	}

	ActivityGraphView*	fCpuGraph;
	ActivityGraphView*	fMemGraph;
	ActivityGraphView*	fNetGraph;
	BStringView*		fCpuInfoText;
	BStringView*		fMemInfoText;
	BStringView*		fNetInfoText;
	SystemStats*		fStats;
};


// ---------------------------------------------------------------------------
// PerformanceView
// ---------------------------------------------------------------------------

PerformanceView::PerformanceView()
	: BView("PerformanceView", B_WILL_DRAW | B_PULSE_NEEDED | B_SUPPORTS_LAYOUT)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BSplitView* splitView = new BSplitView(B_HORIZONTAL, B_USE_DEFAULT_SPACING);
	splitView->SetInsets(B_USE_DEFAULT_SPACING);

	fSummaryView = new SummaryView(&fStats);

	BTabView* tabView = new BTabView("tab_view");
	fRightPane = tabView;
	fRightPane->SetExplicitMinSize(BSize(150, B_SIZE_UNSET));
	fRightPane->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

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

	fCPUView->UpdateData();
	fMemView->UpdateData();

	fStats.cpuUsage      = fCPUView->GetCurrentUsage();
	fStats.memoryUsage   = fMemView->GetCurrentUsage();
	fStats.uploadSpeed   = fNetworkView->GetUploadSpeed();
	fStats.downloadSpeed = fNetworkView->GetDownloadSpeed();
	fStats.cpuFrequency  = GetCpuFrequency();

	uint64 used, total, physical;
	GetMemoryUsage(used, total, physical);
	fStats.memoryUsed    = used;
	fStats.memoryTotal   = total;

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
