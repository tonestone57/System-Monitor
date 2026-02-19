#include "Utils.h"
#include <Font.h>
#include <OS.h>
#include <Catalog.h>
#include <DurationFormat.h>
#include <cstring>
#include <vector>
#include <sys/utsname.h>
#include <Screen.h>
#include <fs_info.h>
#include <Volume.h>
#include <FindDirectory.h>
#include <AppFileInfo.h>
#include <File.h>
#include <Path.h>
#include <Directory.h>
#include <Entry.h>
#include <NetworkRoster.h>
#include <NetworkInterface.h>
#include <NetworkAddress.h>
#include <fcntl.h>
#include <unistd.h>

#if defined(__x86_64__) || defined(__i386__)
#include <cpuid.h>
#endif

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Utils"

void FormatBytes(BString& str, uint64 bytes, int precision) {
	FormatBytes(str, (double)bytes, precision);
}

void FormatBytes(BString& str, double bytes, int precision) {
	if (bytes < 1024.0) {
		str.SetToFormat(B_TRANSLATE("%.1f B"), bytes);
		// Optimization: if it is exactly an integer, don't show .0
		if (bytes == (uint64)bytes)
			str.SetToFormat(B_TRANSLATE("%" B_PRIu64 " B"), (uint64)bytes);
		return;
	}

	double kb = bytes / 1024.0;
	if (kb < 1024.0) {
		str.SetToFormat(B_TRANSLATE("%.*f KiB"), precision, kb);
		return;
	}

	double mb = kb / 1024.0;
	if (mb < 1024.0) {
		str.SetToFormat(B_TRANSLATE("%.*f MiB"), precision, mb);
		return;
	}

	double gb = mb / 1024.0;
	str.SetToFormat(B_TRANSLATE("%.*f GiB"), precision, gb);
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
	double speed = (double)bytesDelta * 1000000.0 / microSecondsDelta;
	BString str;
	FormatBytes(str, speed);
	str << "/s";
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
	static BString sCachedBrand;
	if (sCachedBrand.Length() > 0) return sCachedBrand;

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
			if (brandStr.Length() > 0) {
				sCachedBrand = brandStr;
				return sCachedBrand;
			}
		}
	}
#endif
	sCachedBrand = B_TRANSLATE("Unknown CPU");
	return sCachedBrand;
}

BString GetOSVersion()
{
	static BString sCachedVersion;
	if (sCachedVersion.Length() > 0) return sCachedVersion;

	BString revision;
	struct utsname u;
	uname(&u);

	system_info sysInfo;
	if (get_system_info(&sysInfo) == B_OK) {
		revision.SetToFormat(B_TRANSLATE("Haiku %s (hrev%" B_PRId64 ")"),
			u.machine, sysInfo.kernel_version);
	} else {
		revision << u.sysname << " " << u.machine << " " << u.release;
	}
	sCachedVersion = revision;
	return sCachedVersion;
}

BString GetABIVersion()
{
	static BString sCachedABI;
	if (sCachedABI.Length() > 0) return sCachedABI;

	BString abiVersion;
	BPath path;
	if (find_directory(B_BEOS_LIB_DIRECTORY, &path) == B_OK) {
		path.Append("libbe.so");

		BAppFileInfo appFileInfo;
		version_info versionInfo;
		BFile file;
		if (file.SetTo(path.Path(), B_READ_ONLY) == B_OK
			&& appFileInfo.SetTo(&file) == B_OK
			&& appFileInfo.GetVersionInfo(&versionInfo, B_APP_VERSION_KIND) == B_OK
			&& versionInfo.short_info[0] != '\0') {
			abiVersion = versionInfo.short_info;
		}
	}

	if (abiVersion.IsEmpty())
		abiVersion = B_TRANSLATE("Unknown");

	abiVersion << " (" << B_HAIKU_ABI_NAME << ")";
	sCachedABI = abiVersion;
	return sCachedABI;
}

BString GetGPUInfo()
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

void GetPackageCount(BString& out)
{
	auto countPackages = [](const char* path) -> int {
		BDirectory dir(path);
		if (dir.InitCheck() != B_OK) return 0;
		int count = 0;
		BEntry entry;
		while (dir.GetNextEntry(&entry) == B_OK) {
			BPath p;
			entry.GetPath(&p);
			if (p.InitCheck() == B_OK) {
				BString name(p.Leaf());
				if (name.EndsWith(".hpkg")) count++;
			}
		}
		return count;
	};
	int sysPkgs = countPackages("/boot/system/packages");
	int userPkgs = countPackages("/boot/home/config/packages");
	out.SetToFormat("%d (hpkg-system), %d (hpkg-user)", sysPkgs, userPkgs);
}

BString GetLocalIPAddress()
{
	BNetworkRoster& roster = BNetworkRoster::Default();
	BNetworkInterface interface;
	uint32 cookie = 0;
	BString ip = "127.0.0.1";
	while (roster.GetNextInterface(&cookie, interface) == B_OK) {
		if (interface.Flags() & IFF_LOOPBACK) continue;
		if (!(interface.Flags() & IFF_UP)) continue;

		BNetworkInterfaceAddress addr;
		for (int32 i = 0; i < interface.CountAddresses(); i++) {
			if (interface.GetAddressAt(i, addr) == B_OK) {
				if (addr.Address().Family() == AF_INET) {
					ip = addr.Address().ToString();
					goto ip_found;
				}
			}
		}
	}
ip_found:
	return ip;
}

BString GetBatteryCapacity()
{
	int batFd = open("/dev/power/acpi_battery/0/state", O_RDONLY);
	if (batFd >= 0) {
		char buffer[1024];
		ssize_t bytesRead = read(batFd, buffer, sizeof(buffer) - 1);
		close(batFd);

		if (bytesRead > 0) {
			buffer[bytesRead] = '\0';
			BString state(buffer);
			BString capacityStr;
			int32 capacityIndex = state.FindFirst("capacity: ");
			if (capacityIndex >= 0) {
				int32 end = state.FindFirst("\n", capacityIndex);
				if (end < 0) end = state.Length();

				if (end >= capacityIndex + 10) {
					state.CopyInto(capacityStr, capacityIndex + 10, end - (capacityIndex + 10));
					capacityStr.Trim();
				}

				if (!capacityStr.IsEmpty()) {
					capacityStr << "%";
					return capacityStr;
				}
			}
		}
	}
	return BString(B_TRANSLATE("Unknown"));
}

BString GetLocale()
{
	BString locale;
	const char* lang = getenv("LC_ALL");
	if (!lang) lang = getenv("LANG");
	if (lang) locale = lang;
	else locale = "en_US.UTF-8";
	return locale;
}

#if defined(__x86_64__) || defined(__i386__)
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

BString GetCPUFeatures()
{
	static BString sCachedFeatures;
	static bool sFeaturesCached = false;
	if (sFeaturesCached) return sCachedFeatures;

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

	sCachedFeatures = features;
	sFeaturesCached = true;
	return sCachedFeatures;
#else
	sCachedFeatures = B_TRANSLATE("Not available on this architecture");
	sFeaturesCached = true;
	return sCachedFeatures;
#endif
}

BString GetDisplayInfo()
{
	BScreen screen(B_MAIN_SCREEN_ID);
	if (screen.IsValid()) {
		display_mode mode;
		if (screen.GetMode(&mode) == B_OK) {
			BString display;
			float refresh = 60.0;
			if (mode.timing.pixel_clock > 0 && mode.timing.h_total > 0 && mode.timing.v_total > 0) {
				refresh = (double)mode.timing.pixel_clock * 1000.0
					/ (mode.timing.h_total * mode.timing.v_total);
			}
			display.SetToFormat(B_TRANSLATE("%dx%d, %d Hz"), mode.virtual_width,
				mode.virtual_height, (int)(refresh + 0.5));
			return display;
		}
	}
	return BString(B_TRANSLATE("Unknown"));
}

BString GetRootDiskUsage()
{
	fs_info fs;
	if (fs_stat_dev(dev_for_path("/"), &fs) == B_OK) {
		uint64 total = fs.total_blocks * fs.block_size;
		uint64 free = fs.free_blocks * fs.block_size;
		uint64 used = total - free;
		int percent = 0;
		if (total > 0)
			percent = (int)(100.0 * used / total);

		BString usedStr, totalStr;
		FormatBytes(usedStr, used);
		FormatBytes(totalStr, total);

		BString diskStr;
		diskStr.SetToFormat(B_TRANSLATE("%s / %s (%d%%) - %s"), usedStr.String(),
			totalStr.String(), percent, fs.fsh_name);
		return diskStr;
	}
	return BString(B_TRANSLATE("Unknown"));
}
