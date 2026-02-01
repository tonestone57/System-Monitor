#include "Utils.h"
#include <Font.h>
#include <OS.h>
#include <Catalog.h>
#include <cstring>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Utils"

BString FormatBytes(uint64 bytes, int precision) {
	BString str;
	double kb = bytes / 1024.0;
	double mb = kb / 1024.0;
	double gb = mb / 1024.0;

	if (gb >= 1.0) {
		str.SetToFormat("%.*f GiB", precision, gb);
	} else if (mb >= 1.0) {
		str.SetToFormat("%.*f MiB", precision, mb);
	} else if (kb >= 1.0) {
		str.SetToFormat("%.*f KiB", precision, kb);
	} else {
		str.SetToFormat("%" B_PRIu64 " Bytes", bytes);
	}
	return str;
}

BString FormatHertz(uint64 hertz) {
	BString str;
	double ghz = hertz / 1000000000.0;
	double mhz = hertz / 1000000.0;
	double khz = hertz / 1000.0;

	if (ghz >= 1.0) {
		str.SetToFormat("%.2f GHz", ghz);
	} else if (mhz >= 1.0) {
		str.SetToFormat("%.0f MHz", mhz);
	} else if (khz >= 1.0) {
		str.SetToFormat("%.0f KHz", khz);
	} else {
		str.SetToFormat("%" B_PRIu64 " Hz", hertz);
	}
	return str;
}

BString FormatUptime(bigtime_t uptimeMicros) {
	uint32 seconds = uptimeMicros / 1000000;

	uint32 days = seconds / (24 * 3600);
	seconds %= (24 * 3600);
	uint32 hours = seconds / 3600;
	seconds %= 3600;
	uint32 minutes = seconds / 60;
	seconds %= 60;

	BString uptimeStr;
	if (days > 0) {
		uptimeStr.SetToFormat("%u days, %02u:%02u:%02u", days, hours, minutes, seconds);
	} else {
		uptimeStr.SetToFormat("%02u:%02u:%02u", hours, minutes, seconds);
	}
	return uptimeStr;
}

BString FormatSpeed(uint64 bytesDelta, bigtime_t microSecondsDelta)
{
	if (microSecondsDelta <= 0) return "0 B/s";
	double speed = bytesDelta / (microSecondsDelta / 1000000.0);
	double kbs = speed / 1024.0, mbs = kbs / 1024.0;
	BString str;
	if (mbs >= 1.0) str.SetToFormat("%.2f MiB/s", mbs);
	else if (kbs >= 1.0) str.SetToFormat("%.2f KiB/s", kbs);
	else str.SetToFormat("%.0f B/s", speed);
	return str;
}

float GetScaleFactor(const BFont* font) {
	if (!font) return 1.0f;
	float scale = font->Size() / 12.0f;
	if (scale < 1.0f) scale = 1.0f;
	return scale;
}

uint64 GetRoundedCpuSpeed()
{
	system_info info;
	if (get_system_info(&info) == B_OK)
		return info.cpu_clock_speed / 1000000;
	return 0;
}

BString GetCPUBrandString()
{
#if defined(__x86_64__) || defined(__i386__)
	char brand[49] = {};
	uint32 regs[4] = {};
	for (int i = 0; i < 3; ++i) {
		__asm__ volatile("cpuid"
						 : "=a"(regs[0]), "=b"(regs[1]), "=c"(regs[2]), "=d"(regs[3])
						 : "a"(0x80000002 + i));
		memcpy(brand + i * 16, regs, sizeof(regs));
	}
	BString brandStr(brand);
	brandStr.Trim();
	if (brandStr.Length() > 0)
		return brandStr;
#endif
	return B_TRANSLATE("Unknown CPU");
}
