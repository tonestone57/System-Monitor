#ifndef GPUVIEW_H
#define GPUVIEW_H

#include <View.h>
#include <StringView.h>
#include <String.h>
#include <vector>
#include "ActivityGraphView.h"

class GPUView : public BView {
public:
    GPUView();
    virtual ~GPUView();
    
    virtual void AttachedToWindow();
    virtual void Pulse();

    void SetRefreshInterval(bigtime_t interval);

private:
    void CreateLayout();
    void UpdateData();
    
    BStringView* fCardNameValue;
    BStringView* fDriverVersionValue;
    BStringView* fMemorySizeValue;
    BStringView* fResolutionValue;

    std::vector<ActivityGraphView*> fGpuGraphs;
};

#endif // GPUVIEW_H