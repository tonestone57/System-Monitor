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

SysInfoView::SysInfoView(BRect frame)
    : BView(frame, "SysInfoView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW),
      fKernelNameValue(NULL),
      fKernelVersionValue(NULL),
      fKernelBuildValue(NULL),
      fCPUArchValue(NULL),
      fUptimeValue(NULL),
      fCPUModelValue(NULL),
      fMicrocodeValue(NULL),
      fCPUCoresValue(NULL),
      fCPUClockSpeedValue(NULL),
      fL1CacheValue(NULL),
      fL2CacheValue(NULL),
      fL3CacheValue(NULL),
      fCPUFeaturesValue(NULL),
      fGPUTypeValue(NULL),
      fGPUDriverValue(NULL),
      fGPUVRAMValue(NULL),
      fScreenResolutionValue(NULL),
      fTotalRAMValue(NULL),
      fDiskInfoTextView(NULL),
      fDiskInfoScrollView(NULL),
      fMainSectionsBox(NULL)
{
    SetViewColor(255, 255, 255, 255);
    CreateLayout();
}

SysInfoView::~SysInfoView()
{
    // Child views are automatically deleted
}

void SysInfoView::CreateLayout()
{
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

    BFont titleFont(be_bold_font);
    titleFont.SetSize(titleFont.Size() * 1.2);

    // --- OS Section ---
    BStringView* osTitle = new BStringView("os_title", "OPERATING SYSTEM");
    osTitle->SetFont(&titleFont);
    osTitle->SetViewColor(B_TRANSPARENT_COLOR);

    fKernelNameValue = new BStringView("kernel_name", "Kernel Name: N/A");
    fKernelNameValue->SetViewColor(B_TRANSPARENT_COLOR);
    fKernelVersionValue = new BStringView("kernel_version", "Kernel Version: N/A");
    fKernelVersionValue->SetViewColor(B_TRANSPARENT_COLOR);
    fKernelBuildValue = new BStringView("kernel_build", "Build Date/Time: N/A");
    fKernelBuildValue->SetViewColor(B_TRANSPARENT_COLOR);
    fCPUArchValue = new BStringView("cpu_arch", "CPU Architecture: N/A");
    fCPUArchValue->SetViewColor(B_TRANSPARENT_COLOR);
    fUptimeValue = new BStringView("uptime", "System Uptime: N/A");
    fUptimeValue->SetViewColor(B_TRANSPARENT_COLOR);

    // --- CPU Section ---
    BStringView* cpuTitle = new BStringView("cpu_title", "PROCESSOR");
    cpuTitle->SetFont(&titleFont);
    cpuTitle->SetViewColor(B_TRANSPARENT_COLOR);

    fCPUModelValue = new BStringView("cpu_model", "Model: N/A");
    fCPUModelValue->SetViewColor(B_TRANSPARENT_COLOR);
    fMicrocodeValue = new BStringView("cpu_microcode", "Microcode: N/A");
    fMicrocodeValue->SetViewColor(B_TRANSPARENT_COLOR);
    fCPUCoresValue = new BStringView("cpu_cores", "Cores: N/A");
    fCPUCoresValue->SetViewColor(B_TRANSPARENT_COLOR);
    fCPUClockSpeedValue = new BStringView("cpu_clock", "Clock Speed: N/A");
    fCPUClockSpeedValue->SetViewColor(B_TRANSPARENT_COLOR);
    fL1CacheValue = new BStringView("cpu_l1", "L1 Cache (I/D): N/A");
    fL1CacheValue->SetViewColor(B_TRANSPARENT_COLOR);
    fL2CacheValue = new BStringView("cpu_l2", "L2 Cache: N/A");
    fL2CacheValue->SetViewColor(B_TRANSPARENT_COLOR);
    fL3CacheValue = new BStringView("cpu_l3", "L3 Cache: N/A");
    fL3CacheValue->SetViewColor(B_TRANSPARENT_COLOR);
    fCPUSteppingValue = new BStringView("cpu_stepping", "Stepping: N/A");
    fCPUSteppingValue->SetViewColor(B_TRANSPARENT_COLOR);
    fCPUFeaturesValue = new BStringView("cpu_features", "Features: N/A");
    fCPUFeaturesValue->SetViewColor(B_TRANSPARENT_COLOR);
    fCPUFeaturesValue->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

    // --- Graphics Section ---
    BStringView* graphicsTitle = new BStringView("graphics_title", "GRAPHICS");
    graphicsTitle->SetFont(&titleFont);
    graphicsTitle->SetViewColor(B_TRANSPARENT_COLOR);

    fGPUTypeValue = new BStringView("gpu_type", "GPU Type: N/A");
    fGPUTypeValue->SetViewColor(B_TRANSPARENT_COLOR);
    fGPUDriverValue = new BStringView("gpu_driver", "Driver: N/A");
    fGPUDriverValue->SetViewColor(B_TRANSPARENT_COLOR);
    fGPUVRAMValue = new BStringView("gpu_vram", "VRAM: N/A");
    fGPUVRAMValue->SetViewColor(B_TRANSPARENT_COLOR);
    fScreenResolutionValue = new BStringView("screen_resolution", "Resolution: N/A");
    fScreenResolutionValue->SetViewColor(B_TRANSPARENT_COLOR);

    // --- Memory Section ---
    BStringView* memoryTitle = new BStringView("memory_title", "MEMORY");
    memoryTitle->SetFont(&titleFont);
    memoryTitle->SetViewColor(B_TRANSPARENT_COLOR);

    fTotalRAMValue = new BStringView("total_ram", "Total RAM: N/A");
    fTotalRAMValue->SetViewColor(B_TRANSPARENT_COLOR);

    // --- Disk Section ---
    BStringView* diskTitle = new BStringView("disk_title", "DISK VOLUMES");
    diskTitle->SetFont(&titleFont);
    diskTitle->SetViewColor(B_TRANSPARENT_COLOR);

    fDiskInfoTextView = new BTextView("diskInfoTextView");
    fDiskInfoTextView->SetWordWrap(false);
    fDiskInfoTextView->MakeEditable(false);
    fDiskInfoTextView->SetViewColor(B_TRANSPARENT_COLOR);

    BGridLayout* gridLayout = new BGridLayout(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
    BLayoutBuilder::Grid<>(gridLayout)
        .SetInsets(B_USE_DEFAULT_SPACING)
        .Add(osTitle, 0, 0, 2, 1)
        .Add(fKernelNameValue, 0, 1, 1, 1)
        .Add(fKernelVersionValue, 0, 2, 1, 1)
        .Add(fKernelBuildValue, 0, 3, 1, 1)
        .Add(fCPUArchValue, 0, 4, 1, 1)
        .Add(fUptimeValue, 0, 5, 1, 1)
        .Add(BSpaceLayoutItem::CreateVerticalStrut(B_USE_DEFAULT_SPACING * 2), 0, 6, 1, 1)
        .Add(cpuTitle, 0, 7, 2, 1)
        .Add(fCPUModelValue, 0, 8, 1, 1)
        .Add(fMicrocodeValue, 0, 9, 1, 1)
        .Add(fCPUCoresValue, 0, 10, 1, 1)
        .Add(fCPUClockSpeedValue, 0, 11, 1, 1)
        .Add(fL1CacheValue, 0, 12, 1, 1)
        .Add(fL2CacheValue, 0, 13, 1, 1)
        .Add(fL3CacheValue, 0, 14, 1, 1)
        .Add(fCPUSteppingValue, 0, 15, 1, 1)
        .Add(fCPUFeaturesValue, 0, 16, 1, 1)
        .Add(BSpaceLayoutItem::CreateVerticalStrut(B_USE_DEFAULT_SPACING * 2), 0, 17, 1, 1)
        .Add(graphicsTitle, 0, 18, 2, 1)
        .Add(fGPUTypeValue, 0, 19, 1, 1)
        .Add(fGPUDriverValue, 0, 20, 1, 1)
        .Add(fGPUVRAMValue, 0, 21, 1, 1)
        .Add(fScreenResolutionValue, 0, 22, 1, 1)
        .Add(BSpaceLayoutItem::CreateVerticalStrut(B_USE_DEFAULT_SPACING * 2), 0, 23, 1, 1)
        .Add(memoryTitle, 0, 24, 2, 1)
        .Add(fTotalRAMValue, 0, 25, 1, 1)
        .Add(BSpaceLayoutItem::CreateVerticalStrut(B_USE_DEFAULT_SPACING * 2), 0, 26, 1, 1)
        .Add(diskTitle, 0, 27, 2, 1)
        .Add(fDiskInfoTextView, 0, 28, 1, 1)
        .AddGlue(0, 29);

    BView* gridView = new BView("grid_view", 0, gridLayout);
    BScrollView* scrollView = new BScrollView("sysInfoScroller", gridView,
        B_FOLLOW_ALL, 0, false, true);

    BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
        .SetInsets(0)
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

BString SysInfoView::FormatUptime(bigtime_t bootTime) {
    bigtime_t now = system_time();
    bigtime_t uptimeMicros = now - bootTime;
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
#if defined(__x86_64__) || defined(__i386__)
    unsigned int eax, ebx, ecx, edx;
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        int family = (eax >> 8) & 0xf;
        int model = (eax >> 4) & 0xf;
        int stepping = eax & 0xf;
        if (family == 0xf) {
            family += (eax >> 20) & 0xff;
        }
        if (family == 6 || family == 15) {
            model += ((eax >> 16) & 0xf) << 4;
        }
        // Note: fCPUFamilyValue not in header, using model for now
        fCPUSteppingValue->SetText(BString("Stepping: ") << stepping);
    }

    BString features;
    if (hasSSE())     features += "SSE ";
    if (hasSSE2())    features += "SSE2 ";
    if (hasSSE3())    features += "SSE3 ";
    if (hasSSSE3())   features += "SSSE3 ";
    if (hasSSE41())   features += "SSE4.1 ";
    if (hasSSE42())   features += "SSE4.2 ";
    if (hasAVX())     features += "AVX ";
    if (hasAVX2())    features += "AVX2 ";
    if (hasAVX512())  features += "AVX-512 ";
    if (hasAES())     features += "AES-NI ";
    fCPUFeaturesValue->SetText(BString("Features: ") << (features.IsEmpty() ? "None detected" : features));

    for (int i = 0; i < 16; ++i) {
        if (!__get_cpuid_count(4, i, &eax, &ebx, &ecx, &edx)) break;
        if ((eax & 0x1F) == 0) break;

        int level = (eax >> 5) & 0x7;
        int lineSize = (ebx & 0xFFF) + 1;
        int partitions = ((ebx >> 12) & 0x3FF) + 1;
        int ways = ((ebx >> 22) & 0x3FF) + 1;
        int sets = ecx + 1;
        int sizeKB = (ways * partitions * lineSize * sets) / 1024;

        switch (level) {
        case 1:
            fL1CacheValue->SetText(BString("L1 Cache (I/D): ") << sizeKB << " KB");
            break;
        case 2:
            fL2CacheValue->SetText(BString("L2 Cache: ") << sizeKB << " KB");
            break;
        case 3:
            fL3CacheValue->SetText(BString("L3 Cache: ") << sizeKB << " KB");
            break;
        }
    }
#else
    fCPUFeaturesValue->SetText("Features: Feature detection not supported on this architecture.");
#endif
}

void SysInfoView::LoadData() {
    system_info sysInfo;
    if (get_system_info(&sysInfo) != B_OK) {
        const char* errorMsg = "Error fetching system info";
        if (fKernelNameValue) fKernelNameValue->SetText(BString("Kernel Name: ") << errorMsg);
        return;
    }

    // --- OS Info ---
    if (fKernelNameValue) fKernelNameValue->SetText(BString("Kernel Name: ") << sysInfo.kernel_name);

    BString kernelVer;
    kernelVer.SetToFormat("%" B_PRId64 " (API %" B_PRIu32 ")",
                          sysInfo.kernel_version, sysInfo.abi);
    if (fKernelVersionValue) fKernelVersionValue->SetText(BString("Kernel Version: ") << kernelVer);

    char dateTimeStr[64];
    snprintf(dateTimeStr, sizeof(dateTimeStr), "%s %s",
             sysInfo.kernel_build_date, sysInfo.kernel_build_time);
    struct tm build_tm = {};
    if (strptime(dateTimeStr, "%b %d %Y %H:%M:%S", &build_tm)) {
        char isoStr[32];
        strftime(isoStr, sizeof(isoStr), "%Y-%m-%d %H:%M:%S", &build_tm);
        if (fKernelBuildValue) fKernelBuildValue->SetText(BString("Build Date/Time: ") << isoStr);
    } else {
        if (fKernelBuildValue) fKernelBuildValue->SetText(BString("Build Date/Time: ") << dateTimeStr);
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
    if (fCPUArchValue) fCPUArchValue->SetText(BString("CPU Architecture: ") << archStr);
    if (fUptimeValue) fUptimeValue->SetText(BString("System Uptime: ") << FormatUptime(sysInfo.boot_time));

    // --- CPU Info ---
    BString cpuBrand = GetCPUBrandString();
    if (fCPUModelValue) fCPUModelValue->SetText(BString("Model: ") << (cpuBrand.IsEmpty() ? "Unknown CPU" : cpuBrand));

    // Microcode
    BEntry microcodeEntry("/dev/microcode_info");
    if (fMicrocodeValue) {
        if (microcodeEntry.Exists()) {
            int fd = open("/dev/microcode_info", O_RDONLY);
            if (fd >= 0) {
                char buffer[32] = {};
                ssize_t len = read(fd, buffer, sizeof(buffer) - 1);
                close(fd);
                if (len > 0) {
                    buffer[len] = '\0';
                    BString microcodeStr = BString(buffer).Trim();
                    fMicrocodeValue->SetText(BString("Microcode: ") << microcodeStr);
                    if (fMicrocodeValue->Parent())
                        fMicrocodeValue->Parent()->Show();
                } else if (fMicrocodeValue->Parent()) {
                    fMicrocodeValue->Parent()->Hide();
                }
            } else if (fMicrocodeValue->Parent()) {
                fMicrocodeValue->Parent()->Hide();
            }
        } else if (fMicrocodeValue->Parent()) {
            fMicrocodeValue->Parent()->Hide();
        }
    }

    if (fCPUCoresValue) fCPUCoresValue->SetText(BString("Cores: ") << sysInfo.cpu_count);

    cpu_topology_node_info* topology = NULL;
    uint32_t topologyNodeCount = 0;
    if (get_cpu_topology_info(NULL, &topologyNodeCount) == B_OK && topologyNodeCount > 0) {
        topology = new cpu_topology_node_info[topologyNodeCount];
        if (topology == NULL) {
            return;
        }
        uint32_t actualNodeCount = topologyNodeCount;
        if (get_cpu_topology_info(topology, &actualNodeCount) == B_OK) {
            if (topology != nullptr) {
                uint64_t max_freq = 0;
                for (uint32_t i = 0; i < actualNodeCount; i++) {
                    if (topology[i].type == B_TOPOLOGY_CORE) {
                        if (topology[i].data.core.default_frequency > max_freq)
                            max_freq = topology[i].data.core.default_frequency;
                    }
                }
                if (max_freq > 0)
                    fCPUClockSpeedValue->SetText(BString("Clock Speed: ") << FormatHertz(max_freq));
            }
            delete[] topology;
        }
    }

    this->GetCPUInfo(&sysInfo);

    // --- RAM Info ---
    if (fTotalRAMValue) fTotalRAMValue->SetText(BString("Total RAM: ") << FormatBytes((uint64)sysInfo.max_pages * B_PAGE_SIZE));

    // --- Graphics Info ---
    BScreen screen(B_MAIN_SCREEN_ID);
    if (screen.IsValid()) {
        accelerant_device_info deviceInfo;
        if (screen.GetDeviceInfo(&deviceInfo) == B_OK) {
            if (fGPUTypeValue) fGPUTypeValue->SetText(BString("GPU Type: ") << deviceInfo.name);
            if (fGPUDriverValue) fGPUDriverValue->SetText(BString("Driver: ") << deviceInfo.version);
            if (fGPUVRAMValue) fGPUVRAMValue->SetText(BString("VRAM: ") << FormatBytes(deviceInfo.memory));
        } else {
            if (fGPUTypeValue) fGPUTypeValue->SetText("GPU Type: Error getting GPU info");
            if (fGPUDriverValue) fGPUDriverValue->SetText("Driver: N/A");
            if (fGPUVRAMValue) fGPUVRAMValue->SetText("VRAM: N/A");
        }
        display_mode mode;
        if (screen.GetMode(&mode) == B_OK) {
            BString resStr;
            resStr.SetToFormat("%dx%d", mode.virtual_width, mode.virtual_height);
            if (fScreenResolutionValue) fScreenResolutionValue->SetText(BString("Resolution: ") << resStr);
        } else {
            if (fScreenResolutionValue) fScreenResolutionValue->SetText("Resolution: N/A");
        }
    } else {
        if (fGPUTypeValue) fGPUTypeValue->SetText("GPU Type: Error: Invalid screen object");
        if (fGPUDriverValue) fGPUDriverValue->SetText("Driver: N/A");
        if (fGPUVRAMValue) fGPUVRAMValue->SetText("VRAM: N/A");
        if (fScreenResolutionValue) fScreenResolutionValue->SetText("Resolution: N/A");
    }

    // --- Disk Info ---
    BString diskTextData;
    BVolume volume;
    BVolumeRoster volRoster;
    volRoster.Rewind();
    int diskCount = 0;
    while (volRoster.GetNextVolume(&volume) == B_OK) {
        if (volume.Capacity() <= 0) continue;
        diskCount++;

        fs_info fsInfo;
        if (fs_stat_dev(volume.Device(), &fsInfo) == B_OK) {
            if (diskTextData.Length() > 0)
                diskTextData << "\n---\n";
            diskTextData << "Volume Name: " << fsInfo.volume_name << "\n";

            BDirectory rootDir;
            if (volume.GetRootDirectory(&rootDir) != B_OK) {
                diskTextData << "Error getting root directory\n";
                continue;
            }
            BEntry entry;
            if (rootDir.GetEntry(&entry) != B_OK) {
                diskTextData << "Error getting root entry\n";
                continue;
            }
            BPath path;
            if (entry.GetPath(&path) != B_OK) {
                diskTextData << "Error getting path\n";
                continue;
            }
            diskTextData << "Mount Point: " << path.Path() << "\n";

            diskTextData << "File System: " << fsInfo.fsh_name << "\n";
            diskTextData << "Total Size: " << FormatBytes(fsInfo.total_blocks * fsInfo.block_size).String() << "\n";
            diskTextData << "Free Size: " << FormatBytes(fsInfo.free_blocks * fsInfo.block_size).String();
        } else {
            if (diskTextData.Length() > 0)
                diskTextData << "\n---\n";
            diskTextData << "Error getting filesystem info for a volume.\n";
        }
    }

    if (diskCount == 0) {
        diskTextData = "No disk volumes found or accessible.";
    }
    if (fDiskInfoTextView) fDiskInfoTextView->SetText(diskTextData.String());
}
