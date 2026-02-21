#include "GPUView.h"
#include "Utils.h"
#include <cstdio>
#include <LayoutBuilder.h>
#include <Box.h>
#include <GridLayout.h>
#include <SpaceLayoutItem.h>
#include <Screen.h>
#include <GraphicsDefs.h>
#include <Catalog.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "GPUView"

GPUView::GPUView()
	: BView("GPUView", B_WILL_DRAW | B_PULSE_NEEDED),
	  fCachedWidth(-1),
	  fCachedHeight(-1)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	CreateLayout();
}

void GPUView::CreateLayout()
{
	// Header
	BStringView* gpuLabel = new BStringView("gpu_header", B_TRANSLATE("GPU 0"));
	BFont headerFont(be_bold_font);
	headerFont.SetSize(headerFont.Size() * 1.5);
	gpuLabel->SetFont(&headerFont);

	fCardNameValue = new BStringView("card_name", B_TRANSLATE("Unknown GPU"));
	fCardNameValue->SetAlignment(B_ALIGN_RIGHT);

	// Not-supported notice (shown in place of graphs)
	BStringView* noticeView = new BStringView("gpu_notice",
		B_TRANSLATE("Real-time GPU utilization monitoring is not supported on this system."));
	noticeView->SetAlignment(B_ALIGN_CENTER);
	BFont noteFont(be_plain_font);
	noteFont.SetSize(noteFont.Size() * 0.9f);
	noticeView->SetFont(&noteFont);

	// Info Grid (static info only — no utilization row)
	BGridLayout* infoGrid = new BGridLayout(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
	infoGrid->SetInsets(0, B_USE_DEFAULT_SPACING, 0, 0);

	infoGrid->AddView(new BStringView(NULL, B_TRANSLATE("GPU Memory")), 0, 0);
	fMemorySizeValue = new BStringView("mem_val", B_TRANSLATE("N/A"));
	infoGrid->AddView(fMemorySizeValue, 0, 1);

	infoGrid->AddView(new BStringView(NULL, B_TRANSLATE("Driver Version")), 1, 0);
	fDriverVersionValue = new BStringView("driver_val", B_TRANSLATE("N/A"));
	infoGrid->AddView(fDriverVersionValue, 1, 1);

	infoGrid->AddView(new BStringView(NULL, B_TRANSLATE("Resolution")), 0, 2);
	fResolutionValue = new BStringView("res_val", B_TRANSLATE("N/A"));
	infoGrid->AddView(fResolutionValue, 0, 3);

	// Utilization placeholder (unused, kept for ABI compatibility)
	fUtilizationValue = new BStringView("util_val", "");
	fUtilizationValue->Hide();

	infoGrid->SetColumnWeight(0, 1.0f);
	infoGrid->SetColumnWeight(1, 1.0f);

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.AddGroup(B_HORIZONTAL)
			.Add(gpuLabel)
			.AddGlue()
			.Add(fCardNameValue)
		.End()
		.AddStrut(B_USE_DEFAULT_SPACING)
		.Add(noticeView)
		.AddStrut(B_USE_DEFAULT_SPACING)
		.Add(infoGrid)
		.AddGlue();
}

void GPUView::Pulse() {
	if (IsHidden()) return;

	UpdateData();

	// Haiku currently lacks a generic API for GPU utilization.
	// Graphs remain hidden until driver support is available.
}

GPUView::~GPUView()
{
	// Child views are auto-deleted
}

void GPUView::AttachedToWindow()
{
	BView::AttachedToWindow();
	_UpdateStaticInfo();
	UpdateData();
}

void GPUView::SetRefreshInterval(bigtime_t /*interval*/)
{
	// No graphs to update
}

void GPUView::_UpdateStaticInfo()
{
	BScreen screen(B_MAIN_SCREEN_ID);
	if (!screen.IsValid()) {
		fCardNameValue->SetText(B_TRANSLATE("Error: Invalid screen object"));
		return;
	}

	accelerant_device_info deviceInfo;
	if (screen.GetDeviceInfo(&deviceInfo) == B_OK) {
		fCardNameValue->SetText(deviceInfo.name);
		BString memStr;
		::FormatBytes(memStr, deviceInfo.memory);
		fMemorySizeValue->SetText(memStr.String());

		char versionStr[32];
		snprintf(versionStr, sizeof(versionStr), "%u",
				 static_cast<unsigned int>(deviceInfo.version));
		fDriverVersionValue->SetText(versionStr);

	} else {
		fCardNameValue->SetText(B_TRANSLATE("Unknown"));
		fMemorySizeValue->SetText("-");
		fDriverVersionValue->SetText("-");
	}
}

void GPUView::UpdateData()
{
	BScreen screen(B_MAIN_SCREEN_ID);
	if (!screen.IsValid())
		return;

	display_mode mode;
	if (screen.GetMode(&mode) == B_OK) {
		if (fCachedWidth != mode.virtual_width || fCachedHeight != mode.virtual_height) {
			fCachedWidth = mode.virtual_width;
			fCachedHeight = mode.virtual_height;
			fCachedResolution.SetToFormat("%" B_PRId32 "x%" B_PRId32, fCachedWidth, fCachedHeight);
			fResolutionValue->SetText(fCachedResolution.String());
		}
	} else {
		fResolutionValue->SetText(B_TRANSLATE("N/A"));
	}
}
