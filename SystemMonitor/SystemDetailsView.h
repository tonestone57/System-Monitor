#ifndef SYSTEM_DETAILS_VIEW_H
#define SYSTEM_DETAILS_VIEW_H

#include <View.h>
#include <StringView.h>
#include <TextView.h>
#include <String.h>
#include <NumberFormat.h>
#include <DurationFormat.h>
#include <kernel/OS.h>

class SystemDetailsView : public BView {
public:
    SystemDetailsView();
    virtual ~SystemDetailsView();

    virtual void AttachedToWindow();
    virtual void Pulse();
    virtual void MessageReceived(BMessage* message);

private:
    void _UpdateText(BTextView* textView);

    // Helper methods for fetching data
    BStringView* _CreateLabel(const char* name, const char* text);
    void _UpdateLabel(BStringView* label);
    BStringView* _CreateSubtext(const char* name, const char* text);
    void _UpdateSubtext(BStringView* subtext);

    BString _GetOSVersion();
    BString _GetABIVersion();
    BString _GetCPUCount(system_info* sysInfo);
    BString _GetCPUInfo();
    BString _GetCPUFrequency();
    BString _GetRamSize(system_info* sysInfo);
    BString _GetRamUsage(system_info* sysInfo);
    BString _GetKernelDateTime(system_info* sysInfo);
    BString _GetUptime();

    BStringView* fVersionLabelView;
    BStringView* fVersionInfoView;
    BStringView* fCPULabelView;
    BStringView* fCPUInfoView;
    BStringView* fMemSizeView;
    BStringView* fMemUsageView;
    BStringView* fKernelDateTimeView;
    BTextView* fUptimeView;

    BNumberFormat fNumberFormat;
    BDurationFormat fDurationFormat;

    static const uint8 kLabelCount = 5;
    static const uint8 kSubtextCount = 5;
};

#endif // SYSTEM_DETAILS_VIEW_H
