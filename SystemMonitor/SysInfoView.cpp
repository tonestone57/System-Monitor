#include "SysInfoView.h"
#include "Utils.h"
#include <kernel/OS.h>
#include <Screen.h>
#include <GraphicsDefs.h>
#include <VolumeRoster.h>
#include <Volume.h>
#include <fs_info.h>
#include <stdio.h>
#include <time.h>
#include <TextView.h>
#include <String.h>
#include <Alignment.h>
#include <SpaceLayoutItem.h>
#include <Font.h>
#include <LayoutBuilder.h>
#include <Directory.h>
#include <Path.h>
#include <Box.h>
#include <ScrollView.h>
#include <GridLayout.h>
#include <GroupLayout.h>
#include <Entry.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/utsname.h>

#if defined(__x86_64__) || defined(__i386__)
#include <cpuid.h>
#include <cstring>
#include <cpu_type.h>

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
static const char *kExtendedFeatures[36] = {
    "SSE3", "PCLMULDQ", "DTES64", "MONITOR", "DS-CPL", "VMX", "SMX", "EST",
    "TM2", "SSSE3", "CNTXT-ID", "SDBG", "FMA", "CX16", "xTPR", "PDCM",
    NULL, "PCID", "DCA", "SSE4.1", "SSE4.2", "x2APIC", "MOVEB", "POPCNT",
    "TSC-DEADLINE", "AES", "XSAVE", "OSXSAVE", "AVX", "F16C", "RDRND",
    "HYPERVISOR", "AVX2", "AVX512F", "AVX512DQ", "AVX512VL"
};

/* AMD Extended features leaf 0x80000001 */
static const char *kAMDExtFeatures[32] = {
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, "SCE", NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, "MP", "NX", NULL, "AMD-MMX", NULL,
	"FXSR", "FFXSR", "GBPAGES", "RDTSCP", NULL, "64", "3DNow+", "3DNow!"
};

#endif

#include <InterfaceDefs.h>

SysInfoView::SysInfoView()
    : BView("SysInfoView", B_WILL_DRAW),
      fInfoTextView(NULL)
{
    SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
    CreateLayout();
}

SysInfoView::~SysInfoView()
{
    // Child views are automatically deleted
}

void SysInfoView::CreateLayout()
{
    fInfoTextView = new BTextView("info_text_view");
    fInfoTextView->SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR);
    fInfoTextView->SetStylable(true);
    fInfoTextView->MakeEditable(false);
	fInfoTextView->SetWordWrap(true);

    BScrollView* scrollView = new BScrollView("sysInfoScroller", fInfoTextView,
        false, true, B_PLAIN_BORDER);
    scrollView->SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT));

    BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
        .Add(scrollView)
    .End();
}


void SysInfoView::AttachedToWindow()
{
    BView::AttachedToWindow();
    LoadData();
}

BString SysInfoView::FormatHertz(uint64 hertz) {
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

BString SysInfoView::FormatUptime(bigtime_t uptimeMicros) {
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

BString SysInfoView::GetCPUBrandString()
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
    return BString(brand).Trim();
#else
    return BString("Unknown CPU");
#endif
}

void SysInfoView::LoadData() {
    BString infoText;

    system_info sysInfo;
    if (get_system_info(&sysInfo) != B_OK) {
        infoText << "Error fetching system info";
        fInfoTextView->SetText(infoText.String());
        return;
    }

    // OS Info
    infoText << "OPERATING SYSTEM\n\n";
    struct utsname unameInfo;
    if (uname(&unameInfo) == 0) {
        BString kernelVer;
        kernelVer.SetToFormat("%s %s %s hrev%" B_PRId64 " %s %s",
                              unameInfo.sysname, unameInfo.nodename, unameInfo.version,
                              sysInfo.kernel_version, unameInfo.machine, unameInfo.machine);
        infoText << "Kernel: " << kernelVer << "\n";
    }
    BString archStr;
#if defined(__x86_64__)
    archStr = "x86_64";
#elif defined(__i386__) || defined(__INTEL__)
    archStr = "x86 (32-bit)";
#elif defined(__aarch64__)
    archStr = "ARM64";
#elif defined(__arm__)
    archStr = "ARM (32-bit)";
#elif defined(__riscv)
    archStr = "RISC-V";
#elif defined(__sparc__)
    archStr = "SPARC";
#elif defined(__powerpc__)
    archStr = "PowerPC";
#elif defined(__m68k__)
    archStr = "m68k";
#else
    archStr = "Unknown";
#endif
    infoText << "CPU Architecture: " << archStr << "\n";
    infoText << "System Uptime: " << FormatUptime(system_time()) << "\n\n\n";

    // CPU Info
    infoText << "PROCESSOR\n\n";
    BString cpuBrand = GetCPUBrandString();
    infoText << "Model: " << (cpuBrand.IsEmpty() ? "Unknown CPU" : cpuBrand) << "\n";
    infoText << "Cores: " << sysInfo.cpu_count << "\n";
    infoText << "Features: " << _GetCPUFeaturesString() << "\n";
    cpu_topology_node_info* topology = NULL;
    uint32_t topologyNodeCount = 0;
    if (get_cpu_topology_info(NULL, &topologyNodeCount) == B_OK && topologyNodeCount > 0) {
        topology = new cpu_topology_node_info[topologyNodeCount];
        if (topology != NULL) {
            uint32_t actualNodeCount = topologyNodeCount;
            if (get_cpu_topology_info(topology, &actualNodeCount) == B_OK) {
                uint64_t max_freq = 0;
                for (uint32_t i = 0; i < actualNodeCount; i++) {
                    if (topology[i].type == B_TOPOLOGY_CORE) {
                        if (topology[i].data.core.default_frequency > max_freq)
                            max_freq = topology[i].data.core.default_frequency;
                    }
                }
                if (max_freq > 0)
                    infoText << "Clock Speed: " << FormatHertz(max_freq) << "\n";
            }
            delete[] topology;
        }
    }
    infoText << "\n\n";

    // Graphics Info
    infoText << "GRAPHICS\n\n";
    BScreen screen(B_MAIN_SCREEN_ID);
    if (screen.IsValid()) {
        accelerant_device_info deviceInfo;
        if (screen.GetDeviceInfo(&deviceInfo) == B_OK) {
            infoText << "GPU Type: " << deviceInfo.name << "\n";
            infoText << "Driver: " << deviceInfo.version << "\n";
			if (deviceInfo.memory > 0)
				infoText << "VRAM: " << ::FormatBytes(deviceInfo.memory) << "\n";
			else
				infoText << "VRAM: N/A\n";
        } else {
            infoText << "GPU Type: Error getting GPU info\n";
        }
        display_mode mode;
        if (screen.GetMode(&mode) == B_OK) {
            BString resStr;
            resStr.SetToFormat("%dx%d", mode.virtual_width, mode.virtual_height);
            infoText << "Resolution: " << resStr << "\n";
        } else {
            infoText << "Resolution: N/A\n";
        }
    } else {
        infoText << "GPU Type: Error: Invalid screen object\n";
    }
    infoText << "\n\n";

    // Memory Info
    infoText << "MEMORY\n\n";
    infoText << "Physical RAM: " << ::FormatBytes((uint64)sysInfo.max_pages * B_PAGE_SIZE) << "\n";
    infoText << "Virtual RAM: " << ::FormatBytes((uint64)sysInfo.max_swap_pages * B_PAGE_SIZE) << "\n\n\n";

    // Disk Info
    infoText << "DISK VOLUMES\n\n";
    BVolume volume;
    BVolumeRoster volRoster;
    volRoster.Rewind();
    int diskCount = 0;
    while (volRoster.GetNextVolume(&volume) == B_OK) {
        if (volume.Capacity() <= 0) continue;
        diskCount++;

        fs_info fsInfo;
        if (fs_stat_dev(volume.Device(), &fsInfo) == B_OK) {
            if (diskCount > 1)
                infoText << "\n---\n";
            infoText << "Volume Name: " << fsInfo.volume_name << "\n";

            BDirectory rootDir;
            if (volume.GetRootDirectory(&rootDir) == B_OK) {
                BEntry entry;
                if (rootDir.GetEntry(&entry) == B_OK) {
                    BPath path;
                    if (entry.GetPath(&path) == B_OK) {
                        infoText << "Mount Point: " << path.Path() << "\n";
                    }
                }
            }
            infoText << "File System: " << fsInfo.fsh_name << "\n";
            infoText << "Total Size: " << ::FormatBytes(fsInfo.total_blocks * fsInfo.block_size).String() << "\n";
            infoText << "Free Size: " << ::FormatBytes(fsInfo.free_blocks * fsInfo.block_size).String() << "\n";
        }
    }

    if (diskCount == 0) {
        infoText << "No disk volumes found or accessible.";
    }

    fInfoTextView->SetText(infoText.String());

    BFont font;
    fInfoTextView->GetFont(&font);
    BFont boldFont(be_bold_font);

    fInfoTextView->SetFontAndColor(0, infoText.FindFirst("OPERATING SYSTEM") + 18, &boldFont);
    fInfoTextView->SetFontAndColor(infoText.FindFirst("PROCESSOR"), infoText.FindFirst("PROCESSOR") + 9, &boldFont);
    fInfoTextView->SetFontAndColor(infoText.FindFirst("GRAPHICS"), infoText.FindFirst("GRAPHICS") + 8, &boldFont);
    fInfoTextView->SetFontAndColor(infoText.FindFirst("MEMORY"), infoText.FindFirst("MEMORY") + 6, &boldFont);
    fInfoTextView->SetFontAndColor(infoText.FindFirst("DISK VOLUMES"), infoText.FindFirst("DISK VOLUMES") + 12, &boldFont);
}

BString
SysInfoView::_GetCPUFeaturesString()
{
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

    return features;
#else
    return "Not available on this architecture";
#endif
}
