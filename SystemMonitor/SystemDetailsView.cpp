#include "SystemDetailsView.h"
#include "Utils.h"

#include <cstdio>
#include <time.h>
#include <unistd.h>
#include <cmath>

#include <AppDefs.h>
#include <AppFileInfo.h>
#include <Application.h>
#include <ControlLook.h>
#include <DateTimeFormat.h>
#include <DurationFormat.h>
#include <File.h>
#include <FindDirectory.h>
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
#include <sys/utsname.h>
#include <Screen.h>
#include <GraphicsDefs.h>
#include <fs_info.h>
#include <Volume.h>

#include <Catalog.h>
#include <Locale.h>
#include <LocaleRoster.h>

#include <parsedate.h>

#if defined(__x86_64__) || defined(__i386__)
#include <cpuid.h>
#include <cstring>

/* CPU Features */
static const char *kFeatures[32] = {
	"FPU", "VME", "DE", "PSE",
	"TSC", "MSR", "PAE", "MCE",
	"CX8", "APIC", NULL, "SEP",
	"MTRR", "PGE", "MCA", "CMOV",
	"PAT", "PSE36", "PSN", "CFLUSH",
	NULL, "DS", "ACPI", "MMX",
	"FXSTR", "SSE", "SSE2", "SS",
	"HTT", "TM", "IA64", "PBE",
};

/* CPU Extended features */
static const char *kExtendedFeatures[32] = {
	"SSE3", "PCLMULDQ", "DTES64", "MONITOR", "DS-CPL", "VMX", "SMX", "EST",
	"TM2", "SSSE3", "CNTXT-ID", "SDBG", "FMA", "CX16", "xTPR", "PDCM",
	NULL, "PCID", "DCA", "SSE4.1", "SSE4.2", "x2APIC", "MOVEB", "POPCNT",
	"TSC-DEADLINE", "AES", "XSAVE", "OSXSAVE", "AVX", "F16C", "RDRND",
	"HYPERVISOR"
};

/* Leaf 7, subleaf 0, EBX */
static const char *kLeaf7Features[32] = {
	"FSGSBASE", NULL, NULL, "BMI1", NULL, "AVX2", NULL, "SMEP",
	"BMI2", "ERMS", "INVPCID", NULL, NULL, NULL, NULL, NULL,
	"AVX512F", "AVX512DQ", "RDSEED", "ADX", "SMAP", NULL, NULL, "CLFLUSHOPT",
	NULL, NULL, NULL, NULL, "AVX512CD", "SHA", "AVX512BW", "AVX512VL"
};

/* AMD Extended features leaf 0x80000001 */
static const char *kAMDExtFeatures[32] = {
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, "SCE", NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, "MP", "NX", NULL, "AMD-MMX", NULL,
	"FXSR", "FFXSR", "GBPAGES", "RDTSCP", NULL, "64", "3DNow+", "3DNow!"
};
#endif

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
	BString revision;

	// Fallback to system_info for hrev if available
	system_info sysInfo;
	if (get_system_info(&sysInfo) == B_OK) {
		revision.SetToFormat(B_TRANSLATE_COMMENT("Version: Haiku (hrev%" B_PRId64 ")",
			"Version: Haiku (hrev99999)"), sysInfo.kernel_version);
	} else {
		struct utsname u;
		uname(&u);
		revision << "Version: " << u.sysname << " " << u.release;
	}

	return revision;
}

BString SystemDetailsView::_GetABIVersion()
{
	BString abiVersion;

	// the version is stored in the BEOS:APP_VERSION attribute of libbe.so
	BPath path;
	if (find_directory(B_BEOS_LIB_DIRECTORY, &path) == B_OK) {
		path.Append("libbe.so");

		BAppFileInfo appFileInfo;
		version_info versionInfo;
		BFile file;
		if (file.SetTo(path.Path(), B_READ_ONLY) == B_OK
			&& appFileInfo.SetTo(&file) == B_OK
			&& appFileInfo.GetVersionInfo(&versionInfo,
				B_APP_VERSION_KIND) == B_OK
			&& versionInfo.short_info[0] != '\0')
			abiVersion = versionInfo.short_info;
	}

	if (abiVersion.IsEmpty())
		abiVersion = B_TRANSLATE("Unknown");

	abiVersion << " (" << B_HAIKU_ABI_NAME << ")";

	return abiVersion;
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
	BString clockSpeed;

	uint64 frequency = GetRoundedCpuSpeed();
	if (frequency < 1000) {
		clockSpeed.SetToFormat(B_TRANSLATE_COMMENT("%" B_PRIu64 " MHz",
			"750 Mhz (CPU clock speed)"), frequency);
	}
	else {
		clockSpeed.SetToFormat(B_TRANSLATE_COMMENT("%.2f GHz",
			"3.49 Ghz (CPU clock speed)"), frequency / 1000.0f);
	}

	return clockSpeed;
}

BString SystemDetailsView::_GetCPUFeatures()
{
#if defined(__i386__) || defined(__x86_64__)
	BString features;
	unsigned int eax, ebx, ecx, edx;

	if (__get_cpuid(1, &eax, &ebx, &ecx, &edx) == 1) {
		for (int i = 0; i < 32; i++) {
			if ((edx & (1 << i)) && kFeatures[i]) {
				if (features.Length() > 0)
					features << " ";
				features << kFeatures[i];
			}
		}
		for (int i = 0; i < 32; i++) {
			if ((ecx & (1 << i)) && kExtendedFeatures[i]) {
				if (features.Length() > 0)
					features << " ";
				features << kExtendedFeatures[i];
			}
		}
	}

	if (__get_cpuid(0x80000001, &eax, &ebx, &ecx, &edx) == 1) {
		for (int i = 0; i < 32; i++) {
			if ((edx & (1 << i)) && kAMDExtFeatures[i]) {
				if (features.Length() > 0)
					features << " ";
				features << kAMDExtFeatures[i];
			}
		}
	}

	// Leaf 7 features
	if (__get_cpuid_max(0, NULL) >= 7) {
		if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx) == 1) {
			for (int i = 0; i < 32; i++) {
				if ((ebx & (1 << i)) && kLeaf7Features[i]) {
					if (features.Length() > 0)
						features << " ";
					features << kLeaf7Features[i];
				}
			}
		}
	}

	return features;
#else
	return B_TRANSLATE("Not available on this architecture");
#endif
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
	BString swapUsage;

	// Haiku system_info doesn't easily expose swap usage directly in a simple way
	// without traversing VM pages, but sysInfo->page_faults is there.
	// For basic swap size:
	uint64 swapPages = sysInfo->max_swap_pages;
	uint64 swapUsed = 0; // Not trivially available in system_info

	// Just showing total swap for now
	swapUsage.SetToFormat(B_TRANSLATE("Swap: %" B_PRIu64 " MiB total"),
		(uint64)(swapPages * B_PAGE_SIZE / 1048576.0));

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
	BString uptimeText;

	bigtime_t uptime = system_time();
	bigtime_t now = (bigtime_t)time(NULL) * 1000000;
	fDurationFormat.Format(uptimeText, now - uptime, now);

	return uptimeText;
}

BString SystemDetailsView::_GetGPUInfo()
{
	BScreen screen(B_MAIN_SCREEN_ID);
	if (screen.IsValid()) {
		accelerant_device_info info;
		if (screen.GetDeviceInfo(&info) == B_OK) {
			return BString(info.name);
		}
	}
	return BString(B_TRANSLATE("Unknown"));
}

BString SystemDetailsView::_GetDisplayInfo()
{
	BScreen screen(B_MAIN_SCREEN_ID);
	if (screen.IsValid()) {
		display_mode mode;
		if (screen.GetMode(&mode) == B_OK) {
			BString display;
			float refresh = 60.0;
			if (mode.timing.pixel_clock > 0 && mode.timing.h_total > 0 && mode.timing.v_total > 0)
				refresh = (double)mode.timing.pixel_clock * 1000.0 / (mode.timing.h_total * mode.timing.v_total);

			display.SetToFormat("%dx%d, %d Hz", mode.virtual_width, mode.virtual_height, (int)(refresh + 0.5));
			return display;
		}
	}
	return BString(B_TRANSLATE("Unknown"));
}

BString SystemDetailsView::_GetDiskUsage()
{
	fs_info fs;
	if (fs_stat_dev(dev_for_path("/"), &fs) == B_OK) {
		uint64 total = fs.total_blocks * fs.block_size;
		uint64 free = fs.free_blocks * fs.block_size;
		uint64 used = total - free;
		int percent = (int)(100.0 * used / total);
		BString diskStr;
		BString usedStr, totalStr;
		FormatBytes(usedStr, used);
		FormatBytes(totalStr, total);
		diskStr << usedStr << " / " << totalStr << " (" << percent << "%) - " << fs.fsh_name;
		return diskStr;
	}
	return BString(B_TRANSLATE("Unknown"));
}
