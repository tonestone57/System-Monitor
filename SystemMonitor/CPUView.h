#ifndef CPUVIEW_H
#define CPUVIEW_H

#include <View.h>
#include <StringView.h>
#include <Locker.h>
#include <vector>
#include "ActivityGraphView.h"

class BBox;

class CPUView : public BView {
public:
    CPUView();
    virtual ~CPUView();
    
    virtual void AttachedToWindow();
    virtual void Pulse();
    virtual void Draw(BRect updateRect);

    float GetCurrentUsage();

private:
    void CreateLayout();
    void UpdateData();
    void GetCPUUsage(float& overallUsage);
    
    BStringView* fOverallUsageValue;
    ActivityGraphView* fGraphView;

    BStringView* fSpeedValue;
    BStringView* fProcessesValue;
    BStringView* fThreadsValue;
    BStringView* fUptimeValue;
    
    bigtime_t* fPreviousIdleTime;
    uint32 fCpuCount;
    system_info fPreviousSysInfo;
    
    std::vector<float> fPerCoreUsage;
    
    BLocker fLocker;
    bigtime_t fPreviousTimeSnapshot;
    float fCurrentUsage;
};

#endif // CPUVIEW_H
