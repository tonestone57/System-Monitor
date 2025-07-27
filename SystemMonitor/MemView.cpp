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


// MemView Implementation
MemView::MemView(BRect frame)
    : BView(frame, "MemoryView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_PULSE_NEEDED),
      fCacheGraphView(NULL)
{
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

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

    fCacheGraphView = new GraphView("cacheGraph", (rgb_color){0, 128, 255, 255});
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