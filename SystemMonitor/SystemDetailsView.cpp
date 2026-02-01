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

#include <Catalog.h>
#include <Locale.h>
#include <LocaleRoster.h>

#include <parsedate.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SystemDetailsView"

static int ignored_pages(system_info* sysInfo)
{
	return (int)round(sysInfo->ignored_pages * B_PAGE_SIZE / 1048576.0);
}

static int max_pages(system_info* sysInfo)
{
	return (int)round(sysInfo->max_pages * B_PAGE_SIZE / 1048576.0);
}

static int max_and_ignored_pages(system_info* sysInfo)
{
	return max_pages(sysInfo) + ignored_pages(sysInfo);
}

static int used_pages(system_info* sysInfo)
{
	return (int)round(sysInfo->used_pages * B_PAGE_SIZE / 1048576.0);
}

SystemDetailsView::SystemDetailsView()
    : BView("SystemDetailsView", B_WILL_DRAW | B_PULSE_NEEDED),
      fVersionLabelView(NULL),
      fVersionInfoView(NULL),
      fCPULabelView(NULL),
      fCPUInfoView(NULL),
      fMemSizeView(NULL),
      fMemUsageView(NULL),
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

    // Memory size and usage
    fMemSizeView = _CreateLabel("memlabel", _GetRamSize(&sysInfo));
    fMemUsageView = _CreateSubtext("ramusagetext", _GetRamUsage(&sysInfo));

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
        // Version:
        .Add(fVersionLabelView)
        .Add(fVersionInfoView)
        .AddStrut(offset)
        // Processors:
        .Add(fCPULabelView)
        .Add(fCPUInfoView)
        .AddStrut(offset)
        // Memory:
        .Add(fMemSizeView)
        .Add(fMemUsageView)
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
        ramUsage.SetToFormat(B_TRANSLATE_COMMENT("%d MiB used (%s)",
            "326 MiB used (16%)"), used_pages(sysInfo), data.String());
    } else {
        ramUsage.SetToFormat(B_TRANSLATE_COMMENT("%d MiB used (%d%%)",
            "326 MiB used (16%)"), used_pages(sysInfo), (int)(100 * usedMemoryPercent));
    }

    return ramUsage;
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
