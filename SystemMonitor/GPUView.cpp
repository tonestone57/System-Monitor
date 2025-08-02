#include "GPUView.h"
#include <cstdio>
#include <LayoutBuilder.h>
#include <Box.h>
#include <GridLayout.h>
#include <SpaceLayoutItem.h>
#include <Screen.h>
#include <GraphicsDefs.h>

GPUView::GPUView(BRect frame)
    : BView(frame, "GPUView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
{
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

    BBox* infoBox = new BBox("GPUInfoBox");
    infoBox->SetLabel("Graphics Card Information");

    BGridLayout* grid = new BGridLayout(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
    
    fCardNameLabel = new BStringView("gpu_name_label", "Card Name:");
    fCardNameValue = new BStringView("gpu_name_value", "N/A");
    fChipsetLabel = new BStringView("gpu_chipset_label", "Chipset:");
    fChipsetValue = new BStringView("gpu_chipset_value", "N/A");
    fMemorySizeLabel = new BStringView("gpu_mem_label", "Memory Size:");
    fMemorySizeValue = new BStringView("gpu_mem_value", "N/A");
    fDacSpeedLabel = new BStringView("gpu_dac_label", "DAC Speed:");
    fDacSpeedValue = new BStringView("gpu_dac_value", "N/A");
    fDriverVersionLabel = new BStringView("gpu_driver_label", "Driver Version:");
    fDriverVersionValue = new BStringView("gpu_driver_value", "N/A");

    BLayoutBuilder::Grid<>(grid)
        .SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
        .Add(fCardNameLabel, 0, 0)
        .Add(fCardNameValue, 1, 0)
        .Add(BSpaceLayoutItem::CreateGlue(), 2, 0)

        .Add(fChipsetLabel, 0, 1)
        .Add(fChipsetValue, 1, 1)
        .Add(BSpaceLayoutItem::CreateGlue(), 2, 1)

        .Add(fMemorySizeLabel, 0, 2)
        .Add(fMemorySizeValue, 1, 2)
        .Add(BSpaceLayoutItem::CreateGlue(), 2, 2)

        .Add(fDacSpeedLabel, 0, 3)
        .Add(fDacSpeedValue, 1, 3)
        .Add(BSpaceLayoutItem::CreateGlue(), 2, 3)

        .Add(fDriverVersionLabel, 0, 4)
        .Add(fDriverVersionValue, 1, 4)
        .Add(BSpaceLayoutItem::CreateGlue(), 2, 4);

    grid->SetColumnWeight(2, 1.0f);
    infoBox->SetLayout(grid);

    BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
        .SetInsets(B_USE_DEFAULT_SPACING)
        .Add(infoBox)
        .AddGlue();
}

GPUView::~GPUView()
{
    // Child views are auto-deleted
}

void GPUView::AttachedToWindow()
{
    BView::AttachedToWindow();
    UpdateData();
}

BString GPUView::FormatBytes(uint64 bytes) {
    BString str;
    double mb = bytes / (1024.0 * 1024.0);
    double gb = mb / 1024.0;

    if (gb >= 1.0) {
        str.SetToFormat("%.2f GiB", gb);
    } else {
        str.SetToFormat("%.0f MiB", mb);
    }
    return str;
}

void GPUView::UpdateData()
{
    BScreen screen(B_MAIN_SCREEN_ID);
    if (!screen.IsValid()) {
        fCardNameValue->SetText("Error: Invalid screen object");
        return;
    }

    accelerant_device_info deviceInfo;
    if (screen.GetDeviceInfo(&deviceInfo) == B_OK) {
        fCardNameValue->SetText(deviceInfo.name);
        fChipsetValue->SetText(deviceInfo.chipset);
        fMemorySizeValue->SetText(FormatBytes(deviceInfo.memory));

        char dacSpeedStr[32];
        snprintf(dacSpeedStr, sizeof(dacSpeedStr), "%u MHz", 
                 static_cast<unsigned int>(deviceInfo.dac_speed / 1000));
        fDacSpeedValue->SetText(dacSpeedStr);

        char versionStr[32];
        snprintf(versionStr, sizeof(versionStr), "%u", 
                 static_cast<unsigned int>(deviceInfo.version));
        fDriverVersionValue->SetText(versionStr);

    } else {
        fCardNameValue->SetText("Error: Could not get device info");
        fChipsetValue->SetText("-");
        fMemorySizeValue->SetText("-");
        fDacSpeedValue->SetText("-");
        fDriverVersionValue->SetText("-");
    }
}