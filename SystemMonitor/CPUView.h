#ifndef CPUVIEW_H
#define CPUVIEW_H

#include <View.h>
#include <StringView.h>
#include <Locker.h>
#include <vector>
#include "GraphView.h"

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
    void GetCPUUsage(float& overallUsage);
    
    BStringView* fOverallUsageValue;
    GraphView* fGraphView;
    
    bigtime_t* fPreviousIdleTime;
    uint32 fCpuCount;
    system_info fPreviousSysInfo;
    
    std::vector<float> fPerCoreUsage;
    
    BLocker fLocker;
    bigtime_t fPreviousTimeSnapshot;
};

#endif // CPUVIEW_H
