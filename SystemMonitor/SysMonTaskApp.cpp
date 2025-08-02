#include <Application.h>
#include <Window.h>
#include <View.h>
#include <Alert.h>
#include <LayoutBuilder.h>
#include <GroupView.h>
#include <SplitView.h>
#include <Button.h>
#include <PictureButton.h>
#include <CardLayout.h>
#include <CardView.h>
#include <TabView.h>

#include "ProcessView.h"
#include "CPUView.h"
#include "MemView.h"
#include "DiskView.h"
#include "NetworkView.h"
#include "GPUView.h"
#include "SysInfoView.h"
#include "SystemStats.h"

// Forward declaration
class MainWindow;

class SysMonTaskApp : public BApplication {
public:
    SysMonTaskApp();
    virtual void ReadyToRun();

private:
    MainWindow* mainWindow;
};

class SummaryView : public BView {
public:
    SummaryView(SystemStats* stats) : BView("SummaryView", B_WILL_DRAW | B_PULSE_NEEDED), fStats(stats) {
        SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

        fCpuGraph = new ActivityGraphView("cpu_summary_graph", (rgb_color){80, 255, 80, 255});
        fMemGraph = new ActivityGraphView("mem_summary_graph", (rgb_color){80, 80, 255, 255});
        fNetGraph = new ActivityGraphView("net_summary_graph", (rgb_color){255, 80, 80, 255});

        BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
            .SetInsets(B_USE_DEFAULT_SPACING)
            .Add(new BStringView("cpu_label", "CPU Usage"))
            .Add(fCpuGraph)
            .Add(new BStringView("mem_label", "Memory Usage"))
            .Add(fMemGraph)
            .Add(new BStringView("net_label", "Network Usage"))
            .Add(fNetGraph)
            .AddGlue();
    }

    virtual void Pulse() {
        if (fStats) {
            fCpuGraph->AddValue(system_time(), fStats->cpuUsage);
            fMemGraph->AddValue(system_time(), fStats->memoryUsage);
            fNetGraph->AddValue(system_time(), fStats->uploadSpeed + fStats->downloadSpeed);
        }
    }

private:
    ActivityGraphView* fCpuGraph;
    ActivityGraphView* fMemGraph;
    ActivityGraphView* fNetGraph;
    SystemStats* fStats;
};

// Performance Tab - combines CPU, Memory, Disk, Network, GPU monitoring
class PerformanceView : public BView {
public:
    PerformanceView();
    virtual void AttachedToWindow();
    virtual void Pulse();

private:
    BSplitView* fSplitView;
    SummaryView* fSummaryView;
    BView* fRightPane;

    SystemStats fStats;
    CPUView* fCPUView;
    MemView* fMemView;
    NetworkView* fNetworkView;
};

PerformanceView::PerformanceView()
    : BView("PerformanceView", B_WILL_DRAW | B_PULSE_NEEDED)
{
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

        BSplitView* splitView = new BSplitView(B_HORIZONTAL, B_USE_DEFAULT_SPACING);
        splitView->SetInsets(B_USE_DEFAULT_SPACING);

        fSummaryView = new SummaryView(&fStats);

        BTabView* tabView = new BTabView("tab_view", B_WIDTH_FROM_WIDEST);
        fRightPane = tabView;

        fCPUView = new CPUView();
        fMemView = new MemView(Bounds());
        fNetworkView = new NetworkView(Bounds());
        BView* diskTab = new DiskView(Bounds());
        BView* gpuTab = new GPUView(Bounds());

        tabView->AddTab(fCPUView);
        tabView->TabAt(0)->SetLabel("CPU");
        tabView->AddTab(fMemView);
        tabView->TabAt(1)->SetLabel("Memory");
        tabView->AddTab(fNetworkView);
        tabView->TabAt(2)->SetLabel("Network");
        tabView->AddTab(diskTab);
        tabView->TabAt(3)->SetLabel("Disk");
        tabView->AddTab(gpuTab);
        tabView->TabAt(4)->SetLabel("GPU");

        splitView->AddChild(fSummaryView);
        splitView->AddChild(fRightPane);

        fSplitView = splitView;

        BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
            .Add(splitView)
            .End();
}

void PerformanceView::AttachedToWindow()
{
    BView::AttachedToWindow();
    float leftWidth = fSplitView->Bounds().Width() * 0.25;
    fSummaryView->SetExplicitPreferredSize(BSize(leftWidth, B_SIZE_UNSET));
}

void PerformanceView::Pulse()
{
    fCPUView->Pulse();
    fMemView->Pulse();
    fNetworkView->Pulse();

    fStats.cpuUsage = fCPUView->GetCurrentUsage();
    fStats.memoryUsage = fMemView->GetCurrentUsage();
    fStats.uploadSpeed = fNetworkView->GetUploadSpeed();
    fStats.downloadSpeed = fNetworkView->GetDownloadSpeed();
}

// Message constants for button switching
const uint32 MSG_SWITCH_TO_PERFORMANCE = 'perf';
const uint32 MSG_SWITCH_TO_PROCESSES = 'proc';
const uint32 MSG_SWITCH_TO_SYSTEM = 'syst';

class MainWindow : public BWindow {
public:
    MainWindow(BRect frame);
    virtual bool QuitRequested();
    virtual void MessageReceived(BMessage* message);

private:
    void SwitchToView(int32 index);
    
    BCardLayout* fCardLayout;
    BButton* fPerformanceButton;
    BButton* fProcessesButton;
    BButton* fSystemButton;
    
    ProcessView* fProcessView;
    PerformanceView* fPerformanceView;
    SysInfoView* fSysInfoView;
    
    int32 fCurrentViewIndex;
};

MainWindow::MainWindow(BRect frame)
    : BWindow(frame, "SysMonTask - Haiku System Monitor", B_TITLED_WINDOW, 
              B_QUIT_ON_WINDOW_CLOSE | B_AUTO_UPDATE_SIZE_LIMITS),
      fCurrentViewIndex(0) {

    // Create button bar
    fPerformanceButton = new BButton("Performance", new BMessage(MSG_SWITCH_TO_PERFORMANCE));
    fProcessesButton = new BButton("Processes", new BMessage(MSG_SWITCH_TO_PROCESSES));
    fSystemButton = new BButton("System", new BMessage(MSG_SWITCH_TO_SYSTEM));
    
    // Set initial button states
    fPerformanceButton->SetValue(B_CONTROL_ON);
    fProcessesButton->SetValue(B_CONTROL_OFF);
    fSystemButton->SetValue(B_CONTROL_OFF);
    

    // Create the main content area with card layout
    BView* contentView = new BView("content", 0);
    fCardLayout = new BCardLayout();
    contentView->SetLayout(fCardLayout);
    
    // Create the three main views
    fPerformanceView = new PerformanceView();
    fProcessView = new ProcessView(BRect(0, 0, 600, 400));
    fSysInfoView = new SysInfoView(BRect(0, 0, 600, 400));
    
    // Add views to card layout
    fCardLayout->AddView(fPerformanceView);
    fCardLayout->AddView(fProcessView);
    fCardLayout->AddView(fSysInfoView);
    
    // Set initial view
    fCardLayout->SetVisibleItem((int32)0);
    
    // Set up main window layout
    BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
        .SetInsets(5)
        .AddGroup(B_HORIZONTAL, 5)
            .Add(fPerformanceButton)
            .Add(fProcessesButton) 
            .Add(fSystemButton)
            .AddGlue()
        .End()
        .Add(fCardLayout)
    .End();
    
    // Configure window
    SetSizeLimits(800, 2000, 600, 1500);
    
    // Set pulse rate for real-time updates
    SetPulseRate(1000000); // 1 second pulse
    
    // Center window on screen
    CenterOnScreen();
}

bool MainWindow::QuitRequested() {
    be_app->PostMessage(B_QUIT_REQUESTED);
    return true;
}

void MainWindow::MessageReceived(BMessage* message) {
    switch (message->what) {
        case MSG_SWITCH_TO_PERFORMANCE:
            SwitchToView(0);
            break;
            
        case MSG_SWITCH_TO_PROCESSES:
            SwitchToView(1);
            break;
            
        case MSG_SWITCH_TO_SYSTEM:
            SwitchToView(2);
            break;
            
        default:
            BWindow::MessageReceived(message);
            break;
    }
}

void MainWindow::SwitchToView(int32 index) {
    if (index == fCurrentViewIndex)
        return;
        
    // Update button states
    fPerformanceButton->SetValue(index == 0 ? B_CONTROL_ON : B_CONTROL_OFF);
    fProcessesButton->SetValue(index == 1 ? B_CONTROL_ON : B_CONTROL_OFF);
    fSystemButton->SetValue(index == 2 ? B_CONTROL_ON : B_CONTROL_OFF);
    
    // Switch the visible view
    fCardLayout->SetVisibleItem(index);
    fCurrentViewIndex = index;
}

SysMonTaskApp::SysMonTaskApp()
    : BApplication("application/x-vnd.HaikuSysMonTask") {
    // Constructor
}

void SysMonTaskApp::ReadyToRun() {
    BRect windowRect(100, 100, 1200, 800);
    mainWindow = new MainWindow(windowRect);
    mainWindow->Show();
}

int main() {
    SysMonTaskApp app;
    app.Run();
    return 0;
}
