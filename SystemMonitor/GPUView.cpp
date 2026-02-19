#include "GPUView.h"
#include "Utils.h"
#include <cstdio>
#include <LayoutBuilder.h>
#include <Box.h>
#include <GridLayout.h>
#include <SpaceLayoutItem.h>
#include <Screen.h>
#include <GraphicsDefs.h>
#include <Catalog.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "GPUView"

GPUView::GPUView()
    : BView("GPUView", B_WILL_DRAW | B_PULSE_NEEDED),
      fCachedWidth(-1),
      fCachedHeight(-1)
{
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    CreateLayout();
}

void GPUView::CreateLayout()
{
    // Header
    BStringView* gpuLabel = new BStringView("gpu_header", "GPU 0");
    BFont headerFont(be_bold_font);
    headerFont.SetSize(headerFont.Size() * 1.5);
    gpuLabel->SetFont(&headerFont);

    fCardNameValue = new BStringView("card_name", "Unknown GPU");
    fCardNameValue->SetAlignment(B_ALIGN_RIGHT);

    // Graph Grid (4 graphs)
    BGridLayout* graphGrid = new BGridLayout(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
    const char* titles[] = {"3D", "Copy", "Video Encode", "Video Decode"};

    for (int i = 0; i < 4; i++) {
        BView* container = new BView("graph_container", B_WILL_DRAW);
        container->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

        ActivityGraphView* graph = new ActivityGraphView("gpu_graph", {0, 0, 0, 0}, B_FAILURE_COLOR);
        graph->SetExplicitMinSize(BSize(100, 60));
        fGpuGraphs.push_back(graph);

        BLayoutBuilder::Group<>(container, B_VERTICAL, 0)
            .Add(new BStringView(NULL, titles[i]))
            .Add(graph)
            .End();

        graphGrid->AddView(container, i % 2, i / 2);
    }

    // Info Grid
    BGridLayout* infoGrid = new BGridLayout(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
    infoGrid->SetInsets(0, B_USE_DEFAULT_SPACING, 0, 0);

    infoGrid->AddView(new BStringView(NULL, B_TRANSLATE("Utilization")), 0, 0);
    fUtilizationValue = new BStringView("util_val", "0%");
    infoGrid->AddView(fUtilizationValue, 0, 1);

    infoGrid->AddView(new BStringView(NULL, B_TRANSLATE("GPU Memory")), 1, 0);
    fMemorySizeValue = new BStringView("mem_val", "N/A");
    infoGrid->AddView(fMemorySizeValue, 1, 1);

    infoGrid->AddView(new BStringView(NULL, B_TRANSLATE("Driver Version")), 0, 2);
    fDriverVersionValue = new BStringView("driver_val", "N/A");
    infoGrid->AddView(fDriverVersionValue, 0, 3);

    infoGrid->AddView(new BStringView(NULL, B_TRANSLATE("Resolution")), 1, 2);
    fResolutionValue = new BStringView("res_val", "N/A");
    infoGrid->AddView(fResolutionValue, 1, 3);

    infoGrid->SetColumnWeight(0, 1.0f);
    infoGrid->SetColumnWeight(1, 1.0f);

    BLayoutBuilder::Group<>(this, B_VERTICAL)
        .SetInsets(B_USE_DEFAULT_SPACING)
        .AddGroup(B_HORIZONTAL)
            .Add(gpuLabel)
            .AddGlue()
            .Add(fCardNameValue)
        .End()
        .Add(graphGrid)
        .Add(infoGrid)
        .AddGlue();
}

void GPUView::Pulse() {
    if (IsHidden()) return;

    UpdateData();

    // Placeholder: Haiku currently lacks a generic API for GPU utilization.
    // Graphs and values are injected with 0 until driver support is available.
    bigtime_t now = system_time();
    for (auto* graph : fGpuGraphs) {
        graph->AddValue(now, 0);
    }
    if (fUtilizationValue)
        fUtilizationValue->SetText("0%");
}

GPUView::~GPUView()
{
    // Child views are auto-deleted
}

void GPUView::AttachedToWindow()
{
    BView::AttachedToWindow();
    _UpdateStaticInfo();
    UpdateData();
}

void GPUView::SetRefreshInterval(bigtime_t interval)
{
    for (auto* graph : fGpuGraphs) {
        if (graph)
            graph->SetRefreshInterval(interval);
    }
}

void GPUView::_UpdateStaticInfo()
{
    BScreen screen(B_MAIN_SCREEN_ID);
    if (!screen.IsValid()) {
        fCardNameValue->SetText(B_TRANSLATE("Error: Invalid screen object"));
        return;
    }

    accelerant_device_info deviceInfo;
    if (screen.GetDeviceInfo(&deviceInfo) == B_OK) {
        fCardNameValue->SetText(deviceInfo.name);
        BString memStr;
        ::FormatBytes(memStr, deviceInfo.memory);
        fMemorySizeValue->SetText(memStr.String());

        char versionStr[32];
        snprintf(versionStr, sizeof(versionStr), "%u", 
                 static_cast<unsigned int>(deviceInfo.version));
        fDriverVersionValue->SetText(versionStr);

    } else {
        fCardNameValue->SetText(B_TRANSLATE("Unknown"));
        fMemorySizeValue->SetText("-");
        fDriverVersionValue->SetText("-");
    }
}

void GPUView::UpdateData()
{
    BScreen screen(B_MAIN_SCREEN_ID);
    if (!screen.IsValid())
        return;

    display_mode mode;
    if (screen.GetMode(&mode) == B_OK) {
        if (fCachedWidth != mode.virtual_width || fCachedHeight != mode.virtual_height) {
            fCachedWidth = mode.virtual_width;
            fCachedHeight = mode.virtual_height;
            fCachedResolution.SetToFormat("%" B_PRId32 "x%" B_PRId32, fCachedWidth, fCachedHeight);
            fResolutionValue->SetText(fCachedResolution.String());
        }
    } else {
        fResolutionValue->SetText("N/A");
    }
}
