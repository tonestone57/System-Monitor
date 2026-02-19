#ifndef UTILS_H
#define UTILS_H

#include <String.h>
#include <SupportDefs.h>

class BFont;

void FormatBytes(BString& out, uint64 bytes, int precision = 2);
void GetSwapUsage(uint64& used, uint64& total);
BString FormatHertz(uint64 hertz);
BString FormatUptime(bigtime_t uptimeMicros);
BString FormatSpeed(uint64 bytesDelta, bigtime_t microSecondsDelta);
float GetScaleFactor(const BFont* font);

uint64 GetCpuFrequency();
BString GetCPUBrandString();

#endif // UTILS_H
