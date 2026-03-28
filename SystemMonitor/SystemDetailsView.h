#ifndef SYSTEM_DETAILS_VIEW_H
#define SYSTEM_DETAILS_VIEW_H

#include <View.h>
#include <StringView.h>
#include <TextView.h>
#include <String.h>
#include <NumberFormat.h>
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
	BString _GetCPUFeatures();
	BString _GetRamSize(system_info* sysInfo);
	BString _GetRamUsage(system_info* sysInfo);
	BString _GetSwapUsage(system_info* sysInfo);
	BString _GetKernelDateTime(system_info* sysInfo);
	BString _GetUptime();
	BString _GetGPUInfo();
	BString _GetDisplayInfo();
	BString _GetDiskUsage();

	BStringView* fVersionLabelView;
	BStringView* fVersionInfoView;
	BStringView* fCPULabelView;
	BStringView* fCPUInfoView;
	BTextView*   fCPUFeaturesView;
	BStringView* fMemSizeView;
	BStringView* fMemUsageView;
	BStringView* fSwapUsageView;
	BStringView* fGPUInfoView;
	BStringView* fDisplayInfoView;
	BStringView* fDiskUsageView;
	BStringView* fKernelDateTimeView;
	BTextView*   fUptimeView;

	BNumberFormat fNumberFormat;

	static const uint8 kLabelCount = 9;
	static const uint8 kSubtextCount = 9;
};

#endif // SYSTEM_DETAILS_VIEW_H
