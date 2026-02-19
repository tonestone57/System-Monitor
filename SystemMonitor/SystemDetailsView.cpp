#include "SystemDetailsView.h"
#include "Utils.h"

#include <cstdio>
#include <time.h>
#include <unistd.h>
#include <cmath>

#include <AppDefs.h>
#include <Application.h>
#include <ControlLook.h>
#include <DateTimeFormat.h>
#include <DurationFormat.h>
#include <Font.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <NumberFormat.h>
#include <OS.h>
#include <Path.h>
#include <String.h>
#include <StringFormat.h>
#include <StringView.h>
#include <TextView.h>

#include <Catalog.h>
#include <Locale.h>
#include <LocaleRoster.h>
#include <ScrollView.h>

#include <parsedate.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SystemDetailsView"

static int ignored_pages(system_info* sysInfo)
{
	return (int)round((double)sysInfo->ignored_pages * B_PAGE_SIZE / 1048576.0);
}

static int max_pages(system_info* sysInfo)
{
	return (int)round((double)sysInfo->max_pages * B_PAGE_SIZE / 1048576.0);
}

static int max_and_ignored_pages(system_info* sysInfo)
{
	return max_pages(sysInfo) + ignored_pages(sysInfo);
}

static int used_pages(system_info* sysInfo)
{
	return (int)round((double)sysInfo->used_pages * B_PAGE_SIZE / 1048576.0);
}

SystemDetailsView::SystemDetailsView()
	: BView("SystemDetailsView", B_WILL_DRAW | B_PULSE_NEEDED),
	  fVersionLabelView(NULL),
	  fVersionInfoView(NULL),
	  fCPULabelView(NULL),
	  fCPUInfoView(NULL),
	  fCPUFeaturesView(NULL),
	  fMemSizeView(NULL),
	  fMemUsageView(NULL),
	  fSwapUsageView(NULL),
	  fGPUInfoView(NULL),
	  fDisplayInfoView(NULL),
	  fDiskUsageView(NULL),
	  fKernelDateTimeView(NULL),
	  fUptimeView(NULL)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	// Begin construction of system information controls.
	system_info sysInfo;
	get_system_info(&sysInfo);

	// Create all the various labels for system infomation.

	// OS Version / ABI
	fVersionLabelView = _CreateLabel("oslabel", _GetOSVersion());
	fVersionInfoView = _CreateSubtext("ostext", _GetABIVersion());

	// CPU count, type and clock speed
	fCPULabelView = _CreateLabel("cpulabel", _GetCPUCount(&sysInfo));
	fCPUInfoView = _CreateSubtext("cputext", _GetCPUInfo());

	fCPUFeaturesView = new BTextView("cpu_features");
	fCPUFeaturesView->SetText(_GetCPUFeatures());
	_UpdateText(fCPUFeaturesView);

	// GPU and Display
	BStringView* gpuLabel = _CreateLabel("gpulabel", B_TRANSLATE("GPU:"));
	fGPUInfoView = _CreateSubtext("gputext", _GetGPUInfo());

	BStringView* displayLabel = _CreateLabel("displaylabel", B_TRANSLATE("Display:"));
	fDisplayInfoView = _CreateSubtext("displaytext", _GetDisplayInfo());

	// Memory size and usage
	fMemSizeView = _CreateLabel("memlabel", _GetRamSize(&sysInfo));
	fMemUsageView = _CreateSubtext("ramusagetext", _GetRamUsage(&sysInfo));
	fSwapUsageView = _CreateSubtext("swaptext", _GetSwapUsage(&sysInfo));

	// Disk Usage
	BStringView* diskLabel = _CreateLabel("disklabel", B_TRANSLATE("Disk Usage (Root):"));
	fDiskUsageView = _CreateSubtext("disktext", _GetDiskUsage());

	// Kernel build time/date
	BStringView* kernelLabel = _CreateLabel("kernellabel", B_TRANSLATE("Kernel:"));
	fKernelDateTimeView = _CreateSubtext("kerneltext", _GetKernelDateTime(&sysInfo));

	// Uptime
	BStringView* uptimeLabel = _CreateLabel("uptimelabel", B_TRANSLATE("Time running:"));
	fUptimeView = new BTextView("uptimetext");
	fUptimeView->SetText(_GetUptime());
	_UpdateText(fUptimeView);

	// Now comes the layout

	const float offset = be_control_look->DefaultLabelSpacing();
	const float inset = offset;

	SetLayout(new BGroupLayout(B_VERTICAL, 0));
	BLayoutBuilder::Group<>((BGroupLayout*)GetLayout())
		.Add(new BScrollView("scroll_details", BLayoutBuilder::Group<>(B_VERTICAL)
			// Version:
			.Add(fVersionLabelView)
			.Add(fVersionInfoView)
			.AddStrut(offset)
			// Processors:
			.Add(fCPULabelView)
			.Add(fCPUInfoView)
			.Add(fCPUFeaturesView)
			.AddStrut(offset)
			// GPU/Display:
			.Add(gpuLabel)
			.Add(fGPUInfoView)
			.Add(displayLabel)
			.Add(fDisplayInfoView)
			.AddStrut(offset)
			// Memory:
			.Add(fMemSizeView)
			.Add(fMemUsageView)
			.Add(fSwapUsageView)
			.AddStrut(offset)
			// Disk:
			.Add(diskLabel)
			.Add(fDiskUsageView)
			.AddStrut(offset)
			// Kernel:
			.Add(kernelLabel)
			.Add(fKernelDateTimeView)
			.AddStrut(offset)
			// Time running:
			.Add(uptimeLabel)
			.Add(fUptimeView)
			.AddGlue()
			.SetInsets(inset)
			.View(),
		false, true, B_NO_BORDER))
		.End();
}

SystemDetailsView::~SystemDetailsView()
{
}

void SystemDetailsView::AttachedToWindow()
{
	BView::AttachedToWindow();
}

void SystemDetailsView::Pulse()
{
	system_info sysInfo;
	get_system_info(&sysInfo);

	fMemUsageView->SetText(_GetRamUsage(&sysInfo));
	fSwapUsageView->SetText(_GetSwapUsage(&sysInfo));
	fUptimeView->SetText(_GetUptime());
}

void SystemDetailsView::MessageReceived(BMessage* message)
{
	BView::MessageReceived(message);
}

BStringView* SystemDetailsView::_CreateLabel(const char* name, const char* text)
{
	BStringView* label = new BStringView(name, text);
	_UpdateLabel(label);
	return label;
}

void SystemDetailsView::_UpdateLabel(BStringView* label)
{
	label->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_UNSET));
	label->SetFont(be_bold_font, B_FONT_FAMILY_AND_STYLE);
}

BStringView* SystemDetailsView::_CreateSubtext(const char* name, const char* text)
{
	BStringView* subtext = new BStringView(name, text);
	_UpdateSubtext(subtext);
	return subtext;
}

void SystemDetailsView::_UpdateSubtext(BStringView* subtext)
{
	subtext->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_UNSET));
	subtext->SetFont(be_plain_font, B_FONT_FAMILY_AND_STYLE);
}

void SystemDetailsView::_UpdateText(BTextView* textView)
{
	textView->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_TOP));
	textView->SetFontAndColor(be_plain_font, B_FONT_FAMILY_AND_STYLE);
	textView->SetColorSpace(B_RGBA32);
	textView->MakeResizable(false);
	textView->MakeEditable(false);
	textView->MakeSelectable(false);
	textView->SetWordWrap(true);
	textView->SetDoesUndo(false);
	textView->SetInsets(0, 0, 0, 0);
	textView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
}

BString SystemDetailsView::_GetOSVersion()
{
	BString revision = B_TRANSLATE("Version: ");
	revision << GetOSVersion();
	return revision;
}

BString SystemDetailsView::_GetABIVersion()
{
	return GetABIVersion();
}

BString SystemDetailsView::_GetCPUCount(system_info* sysInfo)
{
	static BStringFormat format(B_TRANSLATE_COMMENT(
		"{0, plural, one{Processor:} other{# Processors:}}",
		"\"Processor:\" or \"2 Processors:\""));

	BString processorLabel;
	format.Format(processorLabel, sysInfo->cpu_count);
	return processorLabel;
}

BString SystemDetailsView::_GetCPUInfo()
{
	BString cpuType = GetCPUBrandString();
	cpuType << " @ " << _GetCPUFrequency();
	return cpuType;
}

BString SystemDetailsView::_GetCPUFrequency()
{
	return ::FormatHertz(GetCpuFrequency());
}

BString SystemDetailsView::_GetCPUFeatures()
{
	return GetCPUFeatures();
}

BString SystemDetailsView::_GetRamSize(system_info* sysInfo)
{
	BString ramSize;
	ramSize.SetToFormat(B_TRANSLATE_COMMENT("%d MiB Memory:",
		"2048 MiB Memory:"), max_and_ignored_pages(sysInfo));

	return ramSize;
}

BString SystemDetailsView::_GetRamUsage(system_info* sysInfo)
{
	BString ramUsage;
	BString data;
	double usedMemoryPercent = double(sysInfo->used_pages) / sysInfo->max_pages;
	status_t status = fNumberFormat.FormatPercent(data, usedMemoryPercent);

	if (status == B_OK) {
		ramUsage.SetToFormat(B_TRANSLATE_COMMENT("RAM: %d MiB used (%s)",
			"RAM: 326 MiB used (16%)"), used_pages(sysInfo), data.String());
	} else {
		ramUsage.SetToFormat(B_TRANSLATE_COMMENT("RAM: %d MiB used (%d%%)",
			"RAM: 326 MiB used (16%)"), used_pages(sysInfo), (int)(100 * usedMemoryPercent));
	}

	return ramUsage;
}

BString SystemDetailsView::_GetSwapUsage(system_info* sysInfo)
{
	uint64 swapUsed, swapTotal;
	GetSwapUsage(swapUsed, swapTotal);

	BString usedStr, totalStr;
	FormatBytes(usedStr, swapUsed);
	FormatBytes(totalStr, swapTotal);

	int percent = 0;
	if (swapTotal > 0)
		percent = (int)(100.0 * swapUsed / swapTotal);

	BString swapUsage;
	swapUsage.SetToFormat(B_TRANSLATE("Swap: %s / %s (%d%%)"),
		usedStr.String(), totalStr.String(), percent);

	return swapUsage;
}

BString SystemDetailsView::_GetKernelDateTime(system_info* sysInfo)
{
	BString kernelDateTime;

	BString buildDateTime;
	buildDateTime << sysInfo->kernel_build_date << " " << sysInfo->kernel_build_time;

	time_t buildDateTimeStamp = parsedate(buildDateTime, -1);

	if (buildDateTimeStamp > 0) {
		if (BDateTimeFormat().Format(kernelDateTime, buildDateTimeStamp,
			B_LONG_DATE_FORMAT, B_MEDIUM_TIME_FORMAT) != B_OK)
			kernelDateTime.SetTo(buildDateTime);
	} else
		kernelDateTime.SetTo(buildDateTime);

	return kernelDateTime;
}

BString SystemDetailsView::_GetUptime()
{
	return ::FormatUptime(system_time());
}

BString SystemDetailsView::_GetGPUInfo()
{
	return GetGPUInfo();
}

BString SystemDetailsView::_GetDisplayInfo()
{
	return GetDisplayInfo();
}

BString SystemDetailsView::_GetDiskUsage()
{
	return GetRootDiskUsage();
}
