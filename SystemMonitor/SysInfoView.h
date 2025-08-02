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
    BString _GetCPUFeaturesString();
    void GetCPUInfo(system_info* sysInfo);
    
    BTextView* fInfoTextView;
};

#endif // SYSINFOVIEW_H