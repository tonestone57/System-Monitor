#ifndef UTILS_H
#define UTILS_H

#include <String.h>
#include <SupportDefs.h>

class BFont;

BString FormatBytes(uint64 bytes, int precision = 2);
BString FormatHertz(uint64 hertz);
BString FormatUptime(bigtime_t uptimeMicros);
BString FormatSpeed(uint64 bytesDelta, bigtime_t microSecondsDelta);
float GetScaleFactor(const BFont* font);

uint64 GetRoundedCpuSpeed();
BString GetCPUBrandString();

#endif // UTILS_H
