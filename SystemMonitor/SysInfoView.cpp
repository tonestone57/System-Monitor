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

// Helper to add a labeled string view to a grid layout
static void AddInfoRow(BGridLayout* grid, int32& row, const char* labelText, BStringView*& valueView) {
    BStringView* labelView = new BStringView(NULL, labelText);
    labelView->SetAlignment(B_ALIGN_RIGHT);
    grid->AddView(labelView, 0, row);
    valueView = new BStringView(NULL, "N/A");
    grid->AddView(valueView, 1, row);
    grid->AddItem(BSpaceLayoutItem::CreateHorizontalStrut(5), 2, row);
    row++;
}

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
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    CreateLayout();
}

SysInfoView::~SysInfoView()
{
    // Child views are automatically deleted
}

void SysInfoView::CreateLayout()
{
    fMainSectionsBox = new BBox(Bounds(), "mainSysInfoBox", B_FOLLOW_ALL_SIDES,
                                 B_WILL_DRAW | B_FRAME_EVENTS, B_PLAIN_BORDER);
    fMainSectionsBox->SetLabel("System Information");

    // --- OS Section ---
    BBox* osBox = new BBox("OSInfo");
    osBox->SetLabel("Operating System");
    BGridLayout* osGrid = new BGridLayout(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
    osGrid->SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
                      B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
    int32 row = 0;
    AddInfoRow(osGrid, row, "Kernel Name:", fKernelNameValue);
    AddInfoRow(osGrid, row, "Kernel Version:", fKernelVersionValue);
    AddInfoRow(osGrid, row, "Build Date/Time:", fKernelBuildValue);
    AddInfoRow(osGrid, row, "CPU Architecture:", fCPUArchValue);
    AddInfoRow(osGrid, row, "System Uptime:", fUptimeValue);
    osBox->SetLayout(osGrid);

    // --- CPU Section ---
    BBox* cpuBox = new BBox("CPUInfo");
    cpuBox->SetLabel("Processor");
    BGridLayout* cpuGrid = new BGridLayout(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
    cpuGrid->SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
                       B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
    row = 0;
    AddInfoRow(cpuGrid, row, "Model:", fCPUModelValue);
    AddInfoRow(cpuGrid, row, "Microcode:", fMicrocodeValue);
    AddInfoRow(cpuGrid, row, "Cores:", fCPUCoresValue);
    AddInfoRow(cpuGrid, row, "Clock Speed:", fCPUClockSpeedValue);
    AddInfoRow(cpuGrid, row, "L1 Cache (I/D):", fL1CacheValue);
    AddInfoRow(cpuGrid, row, "L2 Cache:", fL2CacheValue);
    AddInfoRow(cpuGrid, row, "L3 Cache:", fL3CacheValue);
    AddInfoRow(cpuGrid, row, "Family:", fCPUFamilyValue);
    AddInfoRow(cpuGrid, row, "Model:", fCPUModelValue);
    AddInfoRow(cpuGrid, row, "Stepping:", fCPUSteppingValue);
    AddInfoRow(cpuGrid, row, "Features:", fCPUFeaturesValue);
    fCPUFeaturesValue->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
    cpuBox->SetLayout(cpuGrid);

    // --- Graphics Section ---
    BBox* graphicsBox = new BBox("GraphicsInfo");
    graphicsBox->SetLabel("Graphics");
    BGridLayout* graphicsGrid = new BGridLayout(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
    graphicsGrid->SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
                            B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
    row = 0;
    AddInfoRow(graphicsGrid, row, "GPU Type:", fGPUTypeValue);
    AddInfoRow(graphicsGrid, row, "Driver:", fGPUDriverValue);
    AddInfoRow(graphicsGrid, row, "VRAM:", fGPUVRAMValue);
    AddInfoRow(graphicsGrid, row, "Resolution:", fScreenResolutionValue);
    graphicsBox->SetLayout(graphicsGrid);

    // --- Memory Section ---
    BBox* memoryBox = new BBox("MemoryInfo");
    memoryBox->SetLabel("Memory");
    BGridLayout* memoryGrid = new BGridLayout(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
    memoryGrid->SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
                          B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
    row = 0;
    AddInfoRow(memoryGrid, row, "Total RAM:", fTotalRAMValue);
    memoryBox->SetLayout(memoryGrid);

    // --- Disk Section ---
    BBox* diskBox = new BBox("DiskInfo");
    diskBox->SetLabel("Disk Volumes");
    fDiskInfoTextView = new BTextView("diskInfoTextView");
    fDiskInfoTextView->SetWordWrap(false);
    fDiskInfoTextView->MakeEditable(false);
    fDiskInfoTextView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    fDiskInfoScrollView = new BScrollView("diskInfoScroller", fDiskInfoTextView,
        B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP_BOTTOM, 0, false, true, B_PLAIN_BORDER);
    fDiskInfoScrollView->SetExplicitMinSize(BSize(0, 80));
    BLayoutBuilder::Group<>(diskBox, B_VERTICAL, 0)
        .SetInsets(B_USE_DEFAULT_SPACING)
        .Add(fDiskInfoScrollView);

    // --- Main Layout ---
    BGroupLayout* mainGroupLayout = new BGroupLayout(B_VERTICAL, B_USE_DEFAULT_SPACING);
    fMainSectionsBox->SetLayout(mainGroupLayout);
    font_height fh;
    fMainSectionsBox->GetFontHeight(&fh);
    mainGroupLayout->SetInsets(B_USE_DEFAULT_SPACING,
                               fh.ascent + fh.descent + fh.leading + B_USE_DEFAULT_SPACING,
                               B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
    mainGroupLayout->AddView(osBox);
    mainGroupLayout->AddView(cpuBox);
    mainGroupLayout->AddView(graphicsBox);
    mainGroupLayout->AddView(memoryBox);
    mainGroupLayout->AddView(diskBox);
    BLayoutBuilder::Group<>(mainGroupLayout).AddGlue();

    BScrollView* viewScroller = new BScrollView("sysInfoScroller", fMainSectionsBox,
        B_FOLLOW_ALL_SIDES, 0, false, true, B_NO_BORDER);

    BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
        .SetInsets(0)
        .Add(viewScroller)
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
        fCPUFamilyValue->SetText(BString() << family);
        fCPUModelValue->SetText(BString() << model);
        fCPUSteppingValue->SetText(BString() << stepping);
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
    fCPUFeaturesValue->SetText(features.IsEmpty() ? "None detected" : features);

    for (int i = 0; ; ++i) {
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
            fL1CacheValue->SetText(BString() << sizeKB << " KB");
            break;
        case 2:
            fL2CacheValue->SetText(BString() << sizeKB << " KB");
            break;
        case 3:
            fL3CacheValue->SetText(BString() << sizeKB << " KB");
            break;
        }
    }
#else
    fCPUFeaturesValue->SetText("Feature detection not supported on this architecture.");
#endif
}

#include <stdio.h>

void SysInfoView::LoadData() {
    printf("SysInfoView::LoadData() - entry\n");
    system_info sysInfo;
    if (get_system_info(&sysInfo) != B_OK) {
        const char* errorMsg = "Error fetching system info";
        if (fKernelNameValue) fKernelNameValue->SetText(errorMsg);
        return;
    }
    printf("SysInfoView::LoadData() - got system info\n");

    // --- OS Info ---
    if (fKernelNameValue) fKernelNameValue->SetText(sysInfo.kernel_name);

    BString kernelVer;
    kernelVer.SetToFormat("%" B_PRId64 " (API %" B_PRIu32 ")",
                          sysInfo.kernel_version, sysInfo.abi);
    if (fKernelVersionValue) fKernelVersionValue->SetText(kernelVer);

    char dateTimeStr[64];
    snprintf(dateTimeStr, sizeof(dateTimeStr), "%s %s",
             sysInfo.kernel_build_date, sysInfo.kernel_build_time);
    struct tm build_tm = {};
    if (strptime(dateTimeStr, "%b %d %Y %H:%M:%S", &build_tm)) {
        char isoStr[32];
        strftime(isoStr, sizeof(isoStr), "%Y-%m-%d %H:%M:%S", &build_tm);
        if (fKernelBuildValue) fKernelBuildValue->SetText(isoStr);
    } else {
        if (fKernelBuildValue) fKernelBuildValue->SetText(dateTimeStr);
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
    if (fCPUArchValue) fCPUArchValue->SetText(archStr);
    if (fUptimeValue) fUptimeValue->SetText(FormatUptime(sysInfo.boot_time).String());

    // --- CPU Info ---
    BString cpuBrand = GetCPUBrandString();
    if (fCPUModelValue) fCPUModelValue->SetText(cpuBrand.IsEmpty() ? "Unknown CPU" : cpuBrand);

    // Microcode
    BEntry microcodeEntry("/dev/microcode_info");
    if (microcodeEntry.Exists()) {
        int fd = open("/dev/microcode_info", O_RDONLY);
        if (fd >= 0) {
            char buffer[32] = {};
            ssize_t len = read(fd, buffer, sizeof(buffer) - 1);
            close(fd);
            if (len > 0 && fMicrocodeValue) {
                buffer[len] = '\0';
                BString microcodeStr = BString(buffer).Trim();
                fMicrocodeValue->SetText(microcodeStr);
                fMicrocodeValue->Parent()->Show();
            } else if (fMicrocodeValue) {
                fMicrocodeValue->Parent()->Hide();
            }
        } else if (fMicrocodeValue) {
            fMicrocodeValue->Parent()->Hide();
        }
    } else if (fMicrocodeValue) {
        fMicrocodeValue->Parent()->Hide();
    }

    if (fCPUCoresValue) fCPUCoresValue->SetText(BString() << sysInfo.cpu_count);

    cpu_topology_node_info* topology = NULL;
    uint32_t topologyNodeCount = 0;
    if (get_cpu_topology_info(NULL, &topologyNodeCount) == B_OK) {
        topology = new cpu_topology_node_info[topologyNodeCount];
        if (get_cpu_topology_info(topology, &topologyNodeCount) == B_OK) {
            uint64_t max_freq = 0;
            for (uint32_t i = 0; i < topologyNodeCount; i++) {
                if (topology[i].type == B_TOPOLOGY_CORE) {
                    if (topology[i].data.core.default_frequency > max_freq)
                        max_freq = topology[i].data.core.default_frequency;
                }
            }
            if (max_freq > 0)
                fCPUClockSpeedValue->SetText(FormatHertz(max_freq));
        }
        delete[] topology;
    }

    this->GetCPUInfo(&sysInfo);

    // --- RAM Info ---
    if (fTotalRAMValue) fTotalRAMValue->SetText(FormatBytes((uint64)sysInfo.max_pages * B_PAGE_SIZE).String());

    // --- Graphics Info ---
    printf("SysInfoView::LoadData() - getting graphics info\n");
    BScreen screen(B_MAIN_SCREEN_ID);
    printf("SysInfoView::LoadData() - created BScreen object\n");
    if (screen.IsValid()) {
        printf("SysInfoView::LoadData() - BScreen is valid\n");
        accelerant_device_info deviceInfo;
        if (screen.GetDeviceInfo(&deviceInfo) == B_OK) {
            printf("SysInfoView::LoadData() - got device info\n");
            if (fGPUTypeValue) fGPUTypeValue->SetText(deviceInfo.name);
            if (fGPUDriverValue) fGPUDriverValue->SetText(BString() << deviceInfo.version);
            if (fGPUVRAMValue) fGPUVRAMValue->SetText(FormatBytes(deviceInfo.memory).String());
        } else {
            printf("SysInfoView::LoadData() - failed to get device info\n");
            if (fGPUTypeValue) fGPUTypeValue->SetText("Error getting GPU info");
            if (fGPUDriverValue) fGPUDriverValue->SetText("N/A");
            if (fGPUVRAMValue) fGPUVRAMValue->SetText("N/A");
        }
        display_mode mode;
        if (screen.GetMode(&mode) == B_OK) {
            printf("SysInfoView::LoadData() - got display mode\n");
            BString resStr;
            resStr.SetToFormat("%dx%d", mode.virtual_width, mode.virtual_height);
            if (fScreenResolutionValue) fScreenResolutionValue->SetText(resStr);
        } else {
            printf("SysInfoView::LoadData() - failed to get display mode\n");
            if (fScreenResolutionValue) fScreenResolutionValue->SetText("N/A");
        }
    } else {
        printf("SysInfoView::LoadData() - BScreen is invalid\n");
        if (fGPUTypeValue) fGPUTypeValue->SetText("Error: Invalid screen object");
        if (fGPUDriverValue) fGPUDriverValue->SetText("N/A");
        if (fGPUVRAMValue) fGPUVRAMValue->SetText("N/A");
        if (fScreenResolutionValue) fScreenResolutionValue->SetText("N/A");
    }
    printf("SysInfoView::LoadData() - finished graphics info\n");

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