#ifndef SYSINFOVIEW_H
#define SYSINFOVIEW_H

#include <View.h>
#include <StringView.h>
#include <String.h>
#include <kernel/OS.h>
#include <Messenger.h>

class BBox;
class BTextView;
class BScrollView;

class SysInfoView : public BView {
public:
    SysInfoView();
    virtual ~SysInfoView();
    
    virtual void AttachedToWindow();
    virtual void MessageReceived(BMessage* message);
    virtual void Show();

private:
    void CreateLayout();
    void _StartLoadThread();
    static int32 _LoadDataThread(void* data);
    
    // Static helpers to be used by the thread
    static BString FormatBytes(uint64 bytes, int precision = 2);
    static BString FormatHertz(uint64 hertz);
    static BString FormatUptime(bigtime_t bootTime);
    static BString GetCPUBrandString();
    static BString _GetCPUFeaturesString();
    static void GetCPUInfo(system_info* sysInfo);
    
    BTextView* fLogoTextView;
    BTextView* fInfoTextView;
    thread_id fLoadThread;
};

#endif // SYSINFOVIEW_H
