#include "SystemDetailsView.h"
#include "Utils.h"
#include <cstdlib>

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
#include <GridLayout.h>
#include <SpaceLayoutItem.h>
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

SystemDetailsView::SystemDetailsView()
	: BView("SystemDetailsView", B_WILL_DRAW | B_PULSE_NEEDED),
	  fVersionLabelView(NULL),
	  fVersionInfoView(NULL),
	  fCPULabelView(NULL),
	  fCPUInfoView(NULL),
	  fCPUCoresView(NULL),
	  fCPUFeaturesView(NULL),
	  fMemSizeView(NULL),
	  fMemUsageView(NULL),
	  fCachedUsageView(NULL),
	  fSwapUsageView(NULL),
	  fGPUInfoView(NULL),
	  fDisplayInfoView(NULL),
	  fDiskUsageView(NULL),
	  fKernelDateTimeView(NULL),
	  fUptimeView(NULL)
{
	SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR);

	// Begin construction of system information controls.
	system_info sysInfo;
	get_system_info(&sysInfo);

	// Create all the various labels for system infomation.

	// OS Version / ABI
	fVersionLabelView = _CreateLabel("oslabel", B_TRANSLATE("OS"));
	fVersionInfoView = _CreateSubtext("ostext", _GetOSVersion());

	// CPU count, type and clock speed
	fCPULabelView = _CreateLabel("cpulabel", B_TRANSLATE("CPU"));
	fCPUInfoView = _CreateSubtext("cputext", _GetCPUInfo());
	fCPUCoresView = _CreateSubtext("cpu_cores", _GetCPUCount(&sysInfo));

	fCPUFeaturesView = new BTextView("cpu_features");
	fCPUFeaturesView->SetText(_GetCPUFeatures());
	_UpdateText(fCPUFeaturesView);

	// GPU and Display
	BStringView* gpuLabel = _CreateLabel("gpulabel", B_TRANSLATE("GPU"));
	fGPUInfoView = _CreateSubtext("gputext", _GetGPUInfo());

	BStringView* displayLabel = _CreateLabel("displaylabel", B_TRANSLATE("Resolution"));
	fDisplayInfoView = _CreateSubtext("displaytext", _GetDisplayInfo());

	// Memory size and usage
	fMemSizeView = _CreateLabel("memlabel", _GetRamSize(&sysInfo));
	fMemUsageView = _CreateSubtext("ramusagetext", _GetRamUsage(&sysInfo));
	fCachedUsageView = _CreateSubtext("cachedtext", _GetCachedUsage(&sysInfo));
	fSwapUsageView = _CreateSubtext("swaptext", _GetSwapUsage(&sysInfo));

	// Disk Usage
	BStringView* diskLabel = _CreateLabel("disklabel", B_TRANSLATE("Disk Usage (Root)"));
	fDiskUsageView = _CreateSubtext("disktext", _GetDiskUsage());

	// Kernel build time/date
	BStringView* kernelLabel = _CreateLabel("kernellabel", B_TRANSLATE("Kernel"));
	fKernelDateTimeView = _CreateSubtext("kerneltext", _GetKernelDateTime(&sysInfo));

	// Uptime
	BStringView* uptimeLabel = _CreateLabel("uptimelabel", B_TRANSLATE("Time running"));
	fUptimeView = new BTextView("uptimetext");
	fUptimeView->SetText(_GetUptime());
	_UpdateText(fUptimeView);

	// Now comes the layout

	const float offset = be_control_look->DefaultLabelSpacing();
	const float inset = offset;


	// Packages
	BString packages;
	GetPackageCount(packages);
	fPackagesLabelView = _CreateLabel("packageslabel", B_TRANSLATE("Packages"));
	fPackagesInfoView = _CreateSubtext("packagestext", packages.String());

	// Shell
	const char* shellEnv = getenv("SHELL");
	BString shell = shellEnv ? shellEnv : "/bin/sh";
	BPath shellPath(shell.String());
	if (shellPath.InitCheck() == B_OK) shell = shellPath.Leaf();
	fShellLabelView = _CreateLabel("shelllabel", B_TRANSLATE("Shell"));
	fShellInfoView = _CreateSubtext("shelltext", shell.String());

	// DE / WM
	fDELabelView = _CreateLabel("delabel", B_TRANSLATE("DE"));
	fDEInfoView = _CreateSubtext("detext", B_TRANSLATE("Application Kit"));
	fWMLabelView = _CreateLabel("wmlabel", B_TRANSLATE("WM"));
	fWMInfoView = _CreateSubtext("wmtext", B_TRANSLATE("Application Server"));

	// Font
	font_family family;
	font_style style;
	be_plain_font->GetFamilyAndStyle(&family, &style);
	BString font;
	font << family << " " << style << " (" << static_cast<int>(be_plain_font->Size()) << "pt)";
	fFontLabelView = _CreateLabel("fontlabel", B_TRANSLATE("Font"));
	fFontInfoView = _CreateSubtext("fonttext", font.String());

	// Local IP
	fIPLabelView = _CreateLabel("iplabel", B_TRANSLATE("Local IP"));
	fIPInfoView = _CreateSubtext("iptext", GetLocalIPAddress());

	// Battery
	BString battery = GetBatteryCapacity();
	if (!battery.IsEmpty()) {
		fBatteryLabelView = _CreateLabel("batterylabel", B_TRANSLATE("Battery"));
		fBatteryInfoView = _CreateSubtext("batterytext", battery.String());
	} else {
		fBatteryLabelView = NULL;
		fBatteryInfoView = NULL;
	}

	// Locale
	fLocaleLabelView = _CreateLabel("localelabel", B_TRANSLATE("Locale"));
	fLocaleInfoView = _CreateSubtext("localetext", GetLocale());

BGroupView* detailsGroup = new BGroupView(B_VERTICAL);
	detailsGroup->SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR);

	auto layoutBuilder = BLayoutBuilder::Group<>(detailsGroup, B_VERTICAL)
		.SetInsets(inset)
		.AddGrid(be_control_look->DefaultItemSpacing(), be_control_look->DefaultItemSpacing() / 2)
		// OS Version:
		.Add(fVersionLabelView, 0, 0)
		.Add(fVersionInfoView, 1, 0)
		.Add(BSpaceLayoutItem::CreateGlue(), 2, 0)
		// Kernel:
		.Add(kernelLabel, 0, 1)
		.Add(fKernelDateTimeView, 1, 1)
		// Time running:
		.Add(uptimeLabel, 0, 2)
		.Add(fUptimeView, 1, 2)
		// Packages:
		.Add(fPackagesLabelView, 0, 3)
		.Add(fPackagesInfoView, 1, 3)
		// Shell:
		.Add(fShellLabelView, 0, 4)
		.Add(fShellInfoView, 1, 4)
		// Resolution:
		.Add(displayLabel, 0, 5)
		.Add(fDisplayInfoView, 1, 5)
		// DE:
		.Add(fDELabelView, 0, 6)
		.Add(fDEInfoView, 1, 6)
		// WM:
		.Add(fWMLabelView, 0, 7)
		.Add(fWMInfoView, 1, 7)
		// Font:
		.Add(fFontLabelView, 0, 8)
		.Add(fFontInfoView, 1, 8)
		// CPU / Processors:
		.Add(fCPULabelView, 0, 9)
		.Add(fCPUInfoView, 1, 9)
		.Add(fCPUCoresView, 1, 10)
		.Add(fCPUFeaturesView, 1, 11)
		// GPU:
		.Add(gpuLabel, 0, 12)
		.Add(fGPUInfoView, 1, 12)
		// Memory:
		.Add(fMemSizeView, 0, 13)
		.Add(fMemUsageView, 1, 13)
		.Add(fCachedUsageView, 1, 14)
		.Add(fSwapUsageView, 1, 15)
		// Disk:
		.Add(diskLabel, 0, 16)
		.Add(fDiskUsageView, 1, 16)
		// Local IP:
		.Add(fIPLabelView, 0, 17)
		.Add(fIPInfoView, 1, 17);

	int row = 18;
	if (fBatteryLabelView) {
		layoutBuilder.Add(fBatteryLabelView, 0, row)
			.Add(fBatteryInfoView, 1, row);
		row++;
	}

	layoutBuilder.Add(fLocaleLabelView, 0, row)
		.Add(fLocaleInfoView, 1, row)
		.End()
		.AddGlue()
		.End();

	detailsGroup->SetExplicitMinSize(BSize(B_SIZE_UNSET, 600));

	BScrollView* scrollView = new BScrollView("scroll_details", detailsGroup, 0, false, true);
	scrollView->SetBorder(B_NO_BORDER);
	scrollView->SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT));

	SetLayout(new BGroupLayout(B_VERTICAL, 0));
	BLayoutBuilder::Group<>(static_cast<BGroupLayout*>(GetLayout()))
		.Add(scrollView)
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
	if (IsHidden())
		return;

	system_info sysInfo;
	get_system_info(&sysInfo);

	fMemUsageView->SetText(_GetRamUsage(&sysInfo));
	fCachedUsageView->SetText(_GetCachedUsage(&sysInfo));
	fSwapUsageView->SetText(_GetSwapUsage(&sysInfo));
	fUptimeView->SetText(_GetUptime());

	BString packages;
	GetPackageCount(packages);
	fPackagesInfoView->SetText(packages.String());

	fIPInfoView->SetText(GetLocalIPAddress());

	if (fBatteryInfoView) {
		BString battery = GetBatteryCapacity();
		fBatteryInfoView->SetText(battery.String());
	}

	fLocaleInfoView->SetText(GetLocale());
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
	label->SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT, B_ALIGN_VERTICAL_UNSET));
	BFont font(be_bold_font);
	font.SetSize(font.Size() + 3);
	label->SetFont(&font, B_FONT_ALL);
	label->SetHighColor(139, 0, 0, 255);
	BString text = label->Text();
	text.ToUpper();
	if (!text.EndsWith(":")) {
		text << ":";
	}
	label->SetText(text.String());
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
	BFont font(be_plain_font);
	font.SetSize(font.Size() + 2);
	subtext->SetFont(&font, B_FONT_ALL);
	subtext->SetHighColor(0, 0, 0, 255);
}

void SystemDetailsView::_UpdateText(BTextView* textView)
{
	textView->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_TOP));
	rgb_color black = {0, 0, 0, 255};
	BFont font(be_plain_font);
	font.SetSize(font.Size() + 2);
	textView->SetFontAndColor(&font, B_FONT_ALL, &black);
	textView->SetColorSpace(B_RGBA32);
	textView->MakeResizable(false);
	textView->MakeEditable(false);
	textView->MakeSelectable(false);
	textView->SetWordWrap(true);
	textView->SetDoesUndo(false);
	textView->SetInsets(0, 0, 0, 0);
	textView->SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR);
}

BString SystemDetailsView::_GetOSVersion()
{
	BString revision = GetOSVersion();
	revision << " (" << GetABIVersion() << ")";
	return revision;
}

BString SystemDetailsView::_GetABIVersion()
{
	return GetABIVersion();
}

BString SystemDetailsView::_GetCPUCount(system_info* sysInfo)
{
	static BStringFormat format(B_TRANSLATE_COMMENT(
		"{0, plural, one{# CPU Core} other{# CPU Cores}}",
		"\"1 CPU Core\" or \"6 CPU Cores\""));

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
	return B_TRANSLATE("Memory");
}

BString SystemDetailsView::_GetRamUsage(system_info* sysInfo)
{
	uint64 used, total, physical;
	GetMemoryUsage(used, total, physical);

	BString ramUsage;
	BString data;
	double usedMemoryPercent = total > 0 ? static_cast<double>(used) / total : 0.0;
	status_t status = fNumberFormat.FormatPercent(data, usedMemoryPercent);

	BString usedStr;
	::FormatBytes(usedStr, used);
	BString totalStr;
	::FormatBytes(totalStr, total);

	if (status == B_OK) {
		ramUsage.SetToFormat(B_TRANSLATE_COMMENT("RAM: %s / %s (%s)",
			"RAM: 326.5 MiB / 2048 MiB (16%)"), usedStr.String(), totalStr.String(), data.String());
	} else {
		ramUsage.SetToFormat(B_TRANSLATE_COMMENT("RAM: %s / %s (%d%%)",
			"RAM: 326.5 MiB / 2048 MiB (16%)"), usedStr.String(), totalStr.String(), static_cast<int>(100 * usedMemoryPercent));
	}

	return ramUsage;
}

BString SystemDetailsView::_GetCachedUsage(system_info* sysInfo)
{
	uint64 cachedBytes = static_cast<uint64>(sysInfo->cached_pages + sysInfo->block_cache_pages) * B_PAGE_SIZE;
	BString cachedStr;
	::FormatBytes(cachedStr, cachedBytes);
	BString cachedUsage;
	cachedUsage.SetToFormat(B_TRANSLATE("Cached: %s"), cachedStr.String());
	return cachedUsage;
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
		percent = static_cast<int>(100.0 * swapUsed / swapTotal);

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
