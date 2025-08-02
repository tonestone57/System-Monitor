#ifndef MEMVIEW_H
#define MEMVIEW_H

#include <View.h>
#include <StringView.h>
#include <Locker.h>
#include "GraphView.h"

class BBox;

class MemView : public BView {
public:
    MemView(BRect frame);
    virtual ~MemView();
    
    virtual void AttachedToWindow();
    virtual void Pulse();

    float GetCurrentUsage();

private:
    void UpdateData();
    BString FormatBytes(uint64 bytes);
    
    BStringView* fTotalMemLabel;
    BStringView* fTotalMemValue;
    BStringView* fUsedMemLabel;
    BStringView* fUsedMemValue;
    BStringView* fFreeMemLabel;
    BStringView* fFreeMemValue;
    BStringView* fCachedMemLabel;
    BStringView* fCachedMemValue;
    
    GraphView* fCacheGraphView;
    
    BLocker fLocker;
    float fCurrentUsage;
};

#endif // MEMVIEW_H