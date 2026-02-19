#include "Utils.h"
#include <Font.h>
#include <OS.h>
#include <Catalog.h>
#include <DurationFormat.h>
#include <cstring>
#include <vector>

#if defined(__x86_64__) || defined(__i386__)
#include <cpuid.h>
#endif

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Utils"

void FormatBytes(BString& str, uint64 bytes, int precision) {
	if (bytes < 1024) {
		str.SetToFormat("%" B_PRIu64 " B", bytes);
		return;
	}

	double kb = bytes / 1024.0;
	if (kb < 1024.0) {
		str.SetToFormat("%.*f KiB", precision, kb);
		return;
	}

	double mb = kb / 1024.0;
	if (mb < 1024.0) {
		str.SetToFormat("%.*f MiB", precision, mb);
		return;
	}

	double gb = mb / 1024.0;
	str.SetToFormat("%.*f GiB", precision, gb);
}

void GetSwapUsage(uint64& used, uint64& total) {
	system_info info;
	if (get_system_info(&info) == B_OK) {
		total = (uint64)info.max_swap_pages * B_PAGE_SIZE;
		used = (uint64)info.used_swap_pages * B_PAGE_SIZE;
	} else {
		total = 0;
		used = 0;
	}
}

uint64 GetCachedMemoryBytes(const system_info& sysInfo) {
	// On Haiku, cached memory includes both the page cache and the block cache.
	return ((uint64)sysInfo.cached_pages + (uint64)sysInfo.block_cache_pages) * B_PAGE_SIZE;
}

BString FormatHertz(uint64 hertz) {
	BString str;
	double ghz = hertz / 1000000000.0;
	double mhz = hertz / 1000000.0;
	double khz = hertz / 1000.0;

	if (ghz >= 1.0) {
		str.SetToFormat(B_TRANSLATE("%.2f GHz"), ghz);
	} else if (mhz >= 1.0) {
		str.SetToFormat(B_TRANSLATE("%.0f MHz"), mhz);
	} else if (khz >= 1.0) {
		str.SetToFormat(B_TRANSLATE("%.0f KHz"), khz);
	} else {
		str.SetToFormat(B_TRANSLATE("%" B_PRIu64 " Hz"), hertz);
	}
	return str;
}

BString FormatUptime(bigtime_t uptimeMicros) {
	BString uptimeStr;
	BDurationFormat formatter;
	formatter.Format(uptimeStr, 0, uptimeMicros);
	return uptimeStr;
}

BString FormatSpeed(uint64 bytesDelta, bigtime_t microSecondsDelta)
{
	if (microSecondsDelta <= 0) return B_TRANSLATE("0 B/s");
	double speed = bytesDelta / (microSecondsDelta / 1000000.0);
	double kbs = speed / 1024.0, mbs = kbs / 1024.0;
	BString str;
	if (mbs >= 1.0) str.SetToFormat(B_TRANSLATE("%.2f MiB/s"), mbs);
	else if (kbs >= 1.0) str.SetToFormat(B_TRANSLATE("%.2f KiB/s"), kbs);
	else str.SetToFormat(B_TRANSLATE("%.1f B/s"), speed);
	return str;
}

float GetScaleFactor(const BFont* font) {
	if (!font) return 1.0f;
	float scale = font->Size() / 12.0f;
	if (scale < 1.0f) scale = 1.0f;
	return scale;
}

uint64 GetCpuFrequency()
{
	// Try to get frequency from topology info
	uint32 topologyNodeCount = 0;
	if (get_cpu_topology_info(NULL, &topologyNodeCount) == B_OK && topologyNodeCount > 0) {
		std::vector<cpu_topology_node_info> topology(topologyNodeCount);
		uint32 actualCount = topologyNodeCount;
		if (get_cpu_topology_info(topology.data(), &actualCount) == B_OK) {
			uint64 maxFreq = 0;
			for (uint32 i = 0; i < actualCount; i++) {
				if (topology[i].type == B_TOPOLOGY_CORE) {
					if (topology[i].data.core.default_frequency > maxFreq)
						maxFreq = topology[i].data.core.default_frequency;
				}
			}
			if (maxFreq > 0)
				return maxFreq;
		}
	}

	return 0;
}

BString GetCPUBrandString()
{
#if defined(__x86_64__) || defined(__i386__)
	char brand[49] = {};
	unsigned int regs[4];

	// Check if CPU supports extended function 0x80000000
	if (__get_cpuid(0x80000000, &regs[0], &regs[1], &regs[2], &regs[3])) {
		if (regs[0] >= 0x80000004) {
			for (int i = 0; i < 3; ++i) {
				__get_cpuid(0x80000002 + i, &regs[0], &regs[1], &regs[2], &regs[3]);
				memcpy(brand + i * 16, regs, sizeof(regs));
			}
			BString brandStr(brand);
			brandStr.Trim();
			if (brandStr.Length() > 0)
				return brandStr;
		}
	}
#endif
	return B_TRANSLATE("Unknown CPU");
}
