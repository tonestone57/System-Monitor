#ifndef GPUVIEW_H
#define GPUVIEW_H

#include <View.h>
#include <StringView.h>
#include <String.h>

class GPUView : public BView {
public:
    GPUView();
    virtual ~GPUView();
    
    virtual void AttachedToWindow();

private:
    void UpdateData();
    
    BStringView* fMonitorNameLabel;
    BStringView* fMonitorNameValue;
    BStringView* fResolutionLabel;
    BStringView* fResolutionValue;
    BStringView* fColorDepthLabel;
    BStringView* fColorDepthValue;
    BStringView* fRefreshRateLabel;
    BStringView* fRefreshRateValue;

    BStringView* fCardNameLabel;
    BStringView* fCardNameValue;
    BStringView* fChipsetLabel;
    BStringView* fChipsetValue;
    BStringView* fMemorySizeLabel;
    BStringView* fMemorySizeValue;
    BStringView* fDacSpeedLabel;
    BStringView* fDacSpeedValue;
    BStringView* fDriverVersionLabel;
    BStringView* fDriverVersionValue;
};

#endif // GPUVIEW_H