#ifndef UTILS_H
#define UTILS_H

#include <String.h>
#include <SupportDefs.h>

class BFont;

void FormatBytes(BString& out, uint64 bytes, int precision = 2);
void FormatBytes(BString& out, double bytes, int precision = 2);
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
