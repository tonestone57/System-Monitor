#ifndef SYSINFOVIEW_H
#define SYSINFOVIEW_H

#include <View.h>
#include <StringView.h>
#include <String.h>
#include <kernel/OS.h>

class BBox;
class BTextView;
class BScrollView;

class SysInfoView : public BView {
public:
    SysInfoView(BRect frame);
    virtual ~SysInfoView();
    
    virtual void AttachedToWindow();

private:
    void CreateLayout();
    void LoadData();
    
    BString FormatBytes(uint64 bytes, int precision = 2);
    BString FormatHertz(uint64 hertz);
    BString FormatUptime(bigtime_t bootTime);
    BString GetCPUBrandString();
    BString GetCPUFeatureFlags();
    
    // OS Info
    BStringView* fKernelNameValue;
    BStringView* fKernelVersionValue;
    BStringView* fKernelBuildValue;
    BStringView* fCPUArchValue;
    BStringView* fUptimeValue;
    
    // CPU Info
    BStringView* fCPUModelValue;
    BStringView* fMicrocodeValue;
    BStringView* fCPUCoresValue;
    BStringView* fCPUClockSpeedValue;
    BStringView* fL1CacheValue;
    BStringView* fL2CacheValue;
    BStringView* fL3CacheValue;
    BStringView* fCPUFeaturesValue;
    
    // GPU Info
    BStringView* fGPUTypeValue;
    BStringView* fGPUDriverValue;
    BStringView* fGPUVRAMValue;
    BStringView* fScreenResolutionValue;
    
    // Memory Info
    BStringView* fTotalRAMValue;
    
    // Disk Info
    BTextView* fDiskInfoTextView;
    BScrollView* fDiskInfoScrollView;
    
    BBox* fMainSectionsBox;
};

#endif // SYSINFOVIEW_H