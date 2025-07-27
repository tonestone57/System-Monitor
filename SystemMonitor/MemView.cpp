#include "MemView.h"
#include <Box.h>
#include <GridLayout.h>
#include <GroupLayoutBuilder.h>
#include <SpaceLayoutItem.h>
#include <StringView.h>
#include <stdio.h>
#include <string.h>
#include <kernel/OS.h>
#include <InterfaceDefs.h>
#include <LayoutBuilder.h>
#include <Font.h>

// GraphView Implementation
GraphView::GraphView(BRect frame, const char* name, const float* historyData, const int* historyIndex, const uint64* totalValue)
    : BView(frame, name, B_FOLLOW_ALL_SIDES, B_WILL_DRAW),
      fHistoryData(historyData),
      fHistoryIndex(historyIndex),
      fTotalValue(totalValue) {
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    fGraphColor = (rgb_color){ 0, 128, 255, 255 }; // Blue
}

void GraphView::AttachedToWindow()
{
    BView::AttachedToWindow();
}

void GraphView::Draw(BRect updateRect)
{
    SetLowColor(ViewColor());
    FillRect(updateRect, B_SOLID_LOW);

    if (!fHistoryData || !fHistoryIndex || !fTotalValue || *fTotalValue == 0) {
        BFont font;
        GetFont(&font);
        font_height fh;
        font.GetHeight(&fh);
        const char* message = "Graph N/A";
        float stringWidth = font.StringWidth(message);
        BPoint textPoint(Bounds().Width() / 2 - stringWidth / 2, Bounds().Height() / 2 + fh.ascent / 2);
        DrawString(message, textPoint);
        return;
    }

    SetHighColor(fGraphColor);

    BRect bounds = Bounds();
    float barWidth = bounds.Width() / HISTORY_SIZE;
    int currentIndex = *fHistoryIndex;

    for (int i = 0; i < HISTORY_SIZE; i++) {
        int historyIdx = (currentIndex + i) % HISTORY_SIZE;
        float value = fHistoryData[historyIdx];

        if (value < 0) continue; // Skip uninitialized data points

        float barHeight = (value / 100.0f) * bounds.Height();
        if (barHeight < 0) barHeight = 0;
        if (barHeight > bounds.Height()) barHeight = bounds.Height();

        BRect barRect;
        barRect.left = bounds.left + i * barWidth;
        barRect.right = barRect.left + barWidth - 1;
        barRect.top = bounds.bottom - barHeight;
        barRect.bottom = bounds.bottom;

        FillRect(barRect);
    }

    SetHighColor(0, 0, 0); // Black border
    StrokeRect(bounds);
}

// MemView Implementation
MemView::MemView(BRect frame)
    : BView(frame, "MemoryView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_PULSE_NEEDED),
      fCacheGraphView(NULL),
      fCacheHistoryIndex(0),
      fTotalSystemMemory(0)
{
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    memset(fCacheHistory, -1, sizeof(fCacheHistory)); // Initialize to -1 (uninitialized)

    BBox* statsBox = new BBox("MemoryStatsBox");
    statsBox->SetLabel("Memory Statistics");

    BGridLayout* gridLayout = new BGridLayout(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
    
    fTotalMemLabel = new BStringView("total_mem_label", "Total Memory:");
    fTotalMemValue = new BStringView("total_mem_value", "N/A");
    fUsedMemLabel = new BStringView("used_mem_label", "Used Memory:");
    fUsedMemValue = new BStringView("used_mem_value", "N/A");
    fFreeMemLabel = new BStringView("free_mem_label", "Free Memory:");
    fFreeMemValue = new BStringView("free_mem_value", "N/A");
    fCachedMemLabel = new BStringView("cached_mem_label", "Cached Memory:");
    fCachedMemValue = new BStringView("cached_mem_value", "N/A");

    BLayoutBuilder::Grid<>(gridLayout)
        .SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
        .Add(fTotalMemLabel, 0, 0)
        .Add(fTotalMemValue, 1, 0)
        .Add(BSpaceLayoutItem::CreateGlue(), 2, 0)

        .Add(fUsedMemLabel, 0, 1)
        .Add(fUsedMemValue, 1, 1)
        .Add(BSpaceLayoutItem::CreateGlue(), 2, 1)

        .Add(fFreeMemLabel, 0, 2)
        .Add(fFreeMemValue, 1, 2)
        .Add(BSpaceLayoutItem::CreateGlue(), 2, 2)

        .Add(fCachedMemLabel, 0, 3)
        .Add(fCachedMemValue, 1, 3)
        .Add(BSpaceLayoutItem::CreateGlue(), 2, 3);

    gridLayout->SetColumnWeight(2, 1.0f);
    statsBox->SetLayout(gridLayout);

    fCacheGraphView = new GraphView(BRect(0, 0, 10, 10), "cacheGraph", fCacheHistory, &fCacheHistoryIndex, &fTotalSystemMemory);
    fCacheGraphView->SetExplicitMinSize(BSize(0, 60));
    fCacheGraphView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 100));

    BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
        .SetInsets(B_USE_DEFAULT_SPACING)
        .Add(statsBox)
        .Add(fCacheGraphView)
        .AddGlue();
}

MemView::~MemView()
{
    // Child views are deleted automatically
}

void MemView::AttachedToWindow()
{
    UpdateData();
    BView::AttachedToWindow();
}

void MemView::Pulse()
{
    UpdateData();
}

BString MemView::FormatBytes(uint64 bytes) {
    BString str;
    double kb = bytes / 1024.0;
    double mb = kb / 1024.0;
    double gb = mb / 1024.0;

    if (gb >= 1.0) {
        str.SetToFormat("%.2f GiB", gb);
    } else if (mb >= 1.0) {
        str.SetToFormat("%.2f MiB", mb);
    } else if (kb >= 1.0) {
        str.SetToFormat("%.2f KiB", kb);
    } else {
        str.SetToFormat("%llu Bytes", (unsigned long long)bytes);
    }
    return str;
}

void MemView::UpdateData()
{
    fLocker.Lock();

    system_info sysInfo;
    if (get_system_info(&sysInfo) == B_OK) {
        uint64 totalBytes = (uint64)sysInfo.max_pages * B_PAGE_SIZE;
        uint64 usedBytes = (uint64)sysInfo.used_pages * B_PAGE_SIZE;
        uint64 freeBytes = totalBytes - usedBytes;
        uint64 cachedBytes = ((uint64)sysInfo.cached_pages + (uint64)sysInfo.block_cache_pages) * B_PAGE_SIZE;

        fTotalMemValue->SetText(FormatBytes(totalBytes).String());
        fUsedMemValue->SetText(FormatBytes(usedBytes).String());
        fFreeMemValue->SetText(FormatBytes(freeBytes).String());
        fCachedMemValue->SetText(FormatBytes(cachedBytes).String());

        fTotalSystemMemory = totalBytes;

        if (fTotalSystemMemory > 0) {
            float cachePercent = (float)cachedBytes / fTotalSystemMemory * 100.0f;
            if (cachePercent < 0.0f) cachePercent = 0.0f;
            if (cachePercent > 100.0f) cachePercent = 100.0f;
            fCacheHistory[fCacheHistoryIndex] = cachePercent;
        } else {
            fCacheHistory[fCacheHistoryIndex] = 0;
        }
        fCacheHistoryIndex = (fCacheHistoryIndex + 1) % HISTORY_SIZE;

        if (fCacheGraphView) {
            fCacheGraphView->Invalidate();
        }

    } else {
        fTotalSystemMemory = 0;
        fTotalMemValue->SetText("Error");
        fUsedMemValue->SetText("Error");
        fFreeMemValue->SetText("Error");
        fCachedMemValue->SetText("Error");
    }

    fLocker.Unlock();
}