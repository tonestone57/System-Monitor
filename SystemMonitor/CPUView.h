#ifndef CPUVIEW_H
#define CPUVIEW_H

#include <View.h>
#include <StringView.h>
#include <Locker.h>
#include <vector>
#include "LiveGraphView.h"

class BBox;

class CPUView : public BView {
public:
    CPUView();
    virtual ~CPUView();
    
    virtual void AttachedToWindow();
    virtual void Pulse();
    virtual void Draw(BRect updateRect);

private:
    void CreateLayout();
    void UpdateData();
    void GetCPUUsage(float* overallUsage);
    
    BStringView* fOverallUsageValue;
    LiveGraphView* fGraphView;
    
    bigtime_t* fPreviousIdleTime;
    uint32 fCpuCount;
    system_info fPreviousSysInfo;
    bool fFirstTime;
    
    std::vector<float> fPerCoreUsage;
    
    BLocker fLocker;
};

#endif // CPUVIEW_H
