#include "SysInfoView.h"
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
#include <system_revision.h>

#if defined(__x86_64__) || defined(__i386__)
#include <cpuid.h>
#include <cstring>

bool checkXCR0(unsigned int mask) {
    unsigned int eax, edx;
    asm volatile("xgetbv" : "=a"(eax), "=d"(edx) : "c"(0));
    return (eax & mask) == mask;
}

bool hasSSE()    { unsigned int a,b,c,d; return __get_cpuid(1,&a,&b,&c,&d) && (d & bit_SSE); }
bool hasSSE2()   { unsigned int a,b,c,d; return __get_cpuid(1,&a,&b,&c,&d) && (d & bit_SSE2); }
bool hasSSE3()   { unsigned int a,b,c,d; return __get_cpuid(1,&a,&b,&c,&d) && (c & bit_SSE3); }
bool hasSSSE3()  { unsigned int a,b,c,d; return __get_cpuid(1,&a,&b,&c,&d) && (c & (1 << 9)); }
bool hasSSE41()  { unsigned int a,b,c,d; return __get_cpuid(1,&a,&b,&c,&d) && (c & (1 << 19)); }
bool hasSSE42()  { unsigned int a,b,c,d; return __get_cpuid(1,&a,&b,&c,&d) && (c & (1 << 20)); }

bool hasAVX() {
    unsigned int a,b,c,d;
    if (__get_cpuid(1, &a, &b, &c, &d)) {
        bool avx = c & bit_AVX;
        bool osxsave = c & bit_OSXSAVE;
        return avx && osxsave && checkXCR0(0x6);
    }
    return false;
}

bool hasAVX2() {
    unsigned int a,b,c,d;
    if (__get_cpuid_count(7, 0, &a, &b, &c, &d)) {
        return (b & (1 << 5)) && hasAVX();
    }
    return false;
}

bool hasAVX512() {
    unsigned int a,b,c,d;
    if (__get_cpuid_count(7, 0, &a, &b, &c, &d)) {
        bool avx512f = b & (1 << 16);
        return avx512f && checkXCR0(0xE6);
    }
    return false;
}

bool hasAES() {
    unsigned int eax, ebx, ecx, edx;
    return __get_cpuid(1, &eax, &ebx, &ecx, &edx) && (ecx & bit_AES);
}

#endif // __x86_64__ || __i386__

#include <InterfaceDefs.h>

SysInfoView::SysInfoView(BRect frame)
    : BView(frame, "SysInfoView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW),
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

BString SysInfoView::FormatBytes(uint64 bytes, int precision) {
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

void SysInfoView::GetCPUInfo(system_info* sysInfo)
{
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
    infoText << "Kernel Name: " << sysInfo.kernel_name << "\n";
	BString kernelVer;
	const char* hrev = __get_haiku_revision();
	if (hrev != NULL)
		kernelVer.SetToFormat("%s", hrev);
	else
		kernelVer.SetToFormat("%" B_PRId64, sysInfo.kernel_version);
    infoText << "Kernel Version: " << kernelVer << "\n";
    char dateTimeStr[64];
    snprintf(dateTimeStr, sizeof(dateTimeStr), "%s %s",
             sysInfo.kernel_build_date, sysInfo.kernel_build_time);
    struct tm build_tm = {};
    if (strptime(dateTimeStr, "%b %d %Y %H:%M:%S", &build_tm)) {
        char isoStr[32];
        strftime(isoStr, sizeof(isoStr), "%Y-%m-%d %H:%M:%S", &build_tm);
        infoText << "Build Date/Time: " << isoStr << "\n";
    } else {
        infoText << "Build Date/Time: " << dateTimeStr << "\n";
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
            infoText << "VRAM: " << FormatBytes(deviceInfo.memory) << "\n";
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
    infoText << "Total RAM: " << FormatBytes((uint64)sysInfo.max_pages * B_PAGE_SIZE) << "\n\n\n";

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
            infoText << "Total Size: " << FormatBytes(fsInfo.total_blocks * fsInfo.block_size).String() << "\n";
            infoText << "Free Size: " << FormatBytes(fsInfo.free_blocks * fsInfo.block_size).String() << "\n";
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
