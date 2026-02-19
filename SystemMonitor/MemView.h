#ifndef MEMVIEW_H
#define MEMVIEW_H

#include <View.h>
#include <StringView.h>
#include <Locker.h>
#include "ActivityGraphView.h"

class BBox;

class MemView : public BView {
public:
    MemView();
    virtual ~MemView();
    
    virtual void AttachedToWindow();
    virtual void Pulse();

    float GetCurrentUsage();
    void SetRefreshInterval(bigtime_t interval);
    void UpdateData();

private:
    
    BStringView* fTotalMemLabel;
    BStringView* fTotalMemValue;
    BStringView* fUsedMemLabel;
    BStringView* fUsedMemValue;
    BStringView* fFreeMemLabel;
    BStringView* fFreeMemValue;
    BStringView* fCachedMemLabel;
    BStringView* fCachedMemValue;
    
    ActivityGraphView* fCacheGraphView;
    
    BLocker fLocker;
    float fCurrentUsage;

    uint64 fLastUsedBytes;
    uint64 fLastFreeBytes;
    uint64 fLastCachedBytes;

    BString fCachedUsedStr;
    BString fCachedFreeStr;
    BString fCachedCachedStr;
};

#endif // MEMVIEW_H