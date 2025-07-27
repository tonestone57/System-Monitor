#ifndef MEMVIEW_H
#define MEMVIEW_H

#include <View.h>
#include <StringView.h>
#include <Locker.h>
#include <ScrollView.h>

class BBox;
class GraphView;

#define HISTORY_SIZE 60

class GraphView : public BView {
public:
    GraphView(BRect frame, const char* name, const float* historyData, 
              const int* historyIndex, const uint64* totalValue);
    virtual void AttachedToWindow();
    virtual void Draw(BRect updateRect);

private:
    const float* fHistoryData;
    const int* fHistoryIndex;
    const uint64* fTotalValue;
    rgb_color fGraphColor;
};

class MemView : public BView {
public:
    MemView(BRect frame);
    virtual ~MemView();
    
    virtual void AttachedToWindow();
    virtual void Pulse();

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
    float fCacheHistory[HISTORY_SIZE];
    int fCacheHistoryIndex;
    uint64 fTotalSystemMemory;
    
    BLocker fLocker;
};

#endif // MEMVIEW_H