#include "GPUView.h"
#include "Utils.h"
#include <cstdio>
#include <LayoutBuilder.h>
#include <Box.h>
#include <GridLayout.h>
#include <SpaceLayoutItem.h>
#include <Screen.h>
#include <GraphicsDefs.h>

GPUView::GPUView()
    : BView("GPUView", B_WILL_DRAW)
{
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

    BBox* monitorBox = new BBox("MonitorInfoBox");
    monitorBox->SetLabel("Monitor Information");

    BGridLayout* monitorGrid = new BGridLayout(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);

    fMonitorNameLabel = new BStringView("monitor_name_label", "Name:");
    fMonitorNameValue = new BStringView("monitor_name_value", "N/A");
    fResolutionLabel = new BStringView("monitor_res_label", "Resolution:");
    fResolutionValue = new BStringView("monitor_res_value", "N/A");
    fColorDepthLabel = new BStringView("monitor_color_label", "Color Depth:");
    fColorDepthValue = new BStringView("monitor_color_value", "N/A");
    fRefreshRateLabel = new BStringView("monitor_refresh_label", "Refresh Rate:");
    fRefreshRateValue = new BStringView("monitor_refresh_value", "N/A");

    BLayoutBuilder::Grid<>(monitorGrid)
        .SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
        .Add(fMonitorNameLabel, 0, 0)
        .Add(fMonitorNameValue, 1, 0)
        .Add(BSpaceLayoutItem::CreateGlue(), 2, 0)

        .Add(fResolutionLabel, 0, 1)
        .Add(fResolutionValue, 1, 1)
        .Add(BSpaceLayoutItem::CreateGlue(), 2, 1)

        .Add(fColorDepthLabel, 0, 2)
        .Add(fColorDepthValue, 1, 2)
        .Add(BSpaceLayoutItem::CreateGlue(), 2, 2)

        .Add(fRefreshRateLabel, 0, 3)
        .Add(fRefreshRateValue, 1, 3)
        .Add(BSpaceLayoutItem::CreateGlue(), 2, 3);

    monitorGrid->SetColumnWeight(2, 1.0f);
    monitorBox->SetLayout(monitorGrid);

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
    fDriverVersionLabel = new BStringView("gpu_driver_label", "Driver API Version:");
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
        .Add(monitorBox)
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
        fMemorySizeValue->SetText(::FormatBytes(deviceInfo.memory));

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

    display_mode mode;
    if (screen.GetMode(&mode) == B_OK) {
        BString resStr;
        resStr.SetToFormat("%dx%d", mode.virtual_width, mode.virtual_height);
        fResolutionValue->SetText(resStr);

        int32 bitsPerPixel = 0;
        switch (mode.space) {
            case B_RGB32:
            case B_RGBA32:
            case B_RGB32_BIG:
            case B_RGBA32_BIG:
                bitsPerPixel = 32;
                break;
            case B_RGB24:
            case B_RGB24_BIG:
                bitsPerPixel = 24;
                break;
            case B_RGB16:
            case B_RGB16_BIG:
                bitsPerPixel = 16;
                break;
            case B_RGB15:
            case B_RGBA15:
            case B_RGB15_BIG:
            case B_RGBA15_BIG:
                bitsPerPixel = 15;
                break;
            case B_CMAP8:
                bitsPerPixel = 8;
                break;
            default:
                bitsPerPixel = 0; // Unknown
                break;
        }
        if (bitsPerPixel > 0) {
            BString colorStr;
            colorStr.SetToFormat("%d-bit", bitsPerPixel);
            fColorDepthValue->SetText(colorStr);
        } else {
            fColorDepthValue->SetText("N/A");
        }

        if (mode.timing.h_total > 0 && mode.timing.v_total > 0) {
            double refresh = (double)mode.timing.pixel_clock * 1000.0
                / (mode.timing.h_total * mode.timing.v_total);
            char refreshStr[16];
            snprintf(refreshStr, sizeof(refreshStr), "%.2f Hz", refresh);
            fRefreshRateValue->SetText(refreshStr);
        } else {
            fRefreshRateValue->SetText("N/A");
        }
    } else {
        fResolutionValue->SetText("N/A");
        fColorDepthValue->SetText("N/A");
        fRefreshRateValue->SetText("N/A");
    }

    monitor_info monInfo;
    if (screen.GetMonitorInfo(&monInfo) == B_OK) {
        fMonitorNameValue->SetText(monInfo.name);
    } else {
        fMonitorNameValue->SetText("N/A");
    }
}