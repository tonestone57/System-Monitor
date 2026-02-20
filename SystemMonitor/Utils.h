#ifndef UTILS_H
#define UTILS_H

#include <String.h>
#include <SupportDefs.h>
#include <StringView.h>
#include <vector>
#include <initializer_list>

class BFont;

const uint32 MSG_HEADER_CLICKED = 'head';

class ClickableHeaderView : public BStringView {
public:
	ClickableHeaderView(const char* label, float width, int32 mode, BHandler* target);
	virtual void MouseDown(BPoint where);
	void SetWidth(float width);

private:
	int32 fMode;
	BHandler* fTarget;
};

void FormatBytes(BString& out, uint64 bytes, int precision = 2);
void FormatBytes(BString& out, double bytes, int precision = 2);
uint64 BytesToMiB(uint64 bytes);
void UpdateHeaderWidths(const std::vector<ClickableHeaderView*>& headers, std::initializer_list<float> widths);
void GetMemoryUsage(uint64& used, uint64& total, uint64& physical);
void GetSwapUsage(uint64& used, uint64& total);
uint64 GetCachedMemoryBytes(const system_info& sysInfo);
BString FormatHertz(uint64 hertz);
BString FormatUptime(bigtime_t uptimeMicros);
BString FormatSpeed(uint64 bytesDelta, bigtime_t microSecondsDelta);
float GetScaleFactor(const BFont* font);

uint64 GetCpuFrequency();
BString GetCPUBrandString();

BString GetOSVersion();
BString GetABIVersion();
BString GetGPUInfo();
BString GetDisplayInfo();
BString GetRootDiskUsage();

void GetPackageCount(BString& out);
BString GetLocalIPAddress();
BString GetBatteryCapacity();
BString GetLocale();
BString GetCPUFeatures();

#endif // UTILS_H
