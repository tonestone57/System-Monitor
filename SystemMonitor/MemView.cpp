#include "MemView.h"
#include "Utils.h"
#include <Box.h>
#include <GridLayout.h>
#include <GroupLayoutBuilder.h>
#include <Autolock.h>
#include <SpaceLayoutItem.h>
#include <StringView.h>
#include <cstdio>
#include <string.h>
#include <kernel/OS.h>
#include <InterfaceDefs.h>
#include <LayoutBuilder.h>
#include <Font.h>
#include <Catalog.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MemView"


// MemView Implementation
MemView::MemView()
    : BView("MemoryView", B_WILL_DRAW | B_PULSE_NEEDED),
      fCacheGraphView(NULL),
      fCurrentUsage(0.0f)
{
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

    BBox* statsBox = new BBox("MemoryStatsBox");
    statsBox->SetLabel(B_TRANSLATE("Memory Statistics"));

    BGridLayout* gridLayout = new BGridLayout(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
    
    fTotalMemLabel = new BStringView("total_mem_label", B_TRANSLATE("Total Memory:"));
    fTotalMemValue = new BStringView("total_mem_value", "N/A");
    fUsedMemLabel = new BStringView("used_mem_label", B_TRANSLATE("Used Memory:"));
    fUsedMemValue = new BStringView("used_mem_value", "N/A");
    fFreeMemLabel = new BStringView("free_mem_label", B_TRANSLATE("Free Memory:"));
    fFreeMemValue = new BStringView("free_mem_value", "N/A");
    fCachedMemLabel = new BStringView("cached_mem_label", B_TRANSLATE("Cached Memory:"));
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

    fCacheGraphView = new ActivityGraphView("cacheGraph", {0, 0, 0, 0}, B_MENU_SELECTION_BACKGROUND_COLOR);
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
    BView::AttachedToWindow();
    UpdateData();
}

void MemView::Pulse()
{
    UpdateData();
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

        fTotalMemValue->SetText(::FormatBytes(totalBytes));
        fUsedMemValue->SetText(::FormatBytes(usedBytes));
        fFreeMemValue->SetText(::FormatBytes(freeBytes));
        fCachedMemValue->SetText(::FormatBytes(cachedBytes));

        if (totalBytes > 0) {
            float usedPercent = (float)usedBytes / totalBytes * 100.0f;
            if (usedPercent < 0.0f) usedPercent = 0.0f;
            if (usedPercent > 100.0f) usedPercent = 100.0f;
            fCurrentUsage = usedPercent;

            float cachePercent = (float)cachedBytes / totalBytes * 100.0f;
            if (cachePercent < 0.0f) cachePercent = 0.0f;
            if (cachePercent > 100.0f) cachePercent = 100.0f;
            fCacheGraphView->AddValue(system_time(), cachePercent);
        }

    } else {
        fTotalMemValue->SetText(B_TRANSLATE("Error"));
        fUsedMemValue->SetText(B_TRANSLATE("Error"));
        fFreeMemValue->SetText(B_TRANSLATE("Error"));
        fCachedMemValue->SetText(B_TRANSLATE("Error"));
    }

    fLocker.Unlock();
}

float MemView::GetCurrentUsage()
{
	BAutolock locker(fLocker);
    return fCurrentUsage;
}
