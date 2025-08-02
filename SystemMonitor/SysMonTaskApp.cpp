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

#include "ProcessView.h"
#include "CPUView.h"
#include "MemView.h"
#include "DiskView.h"
#include "NetworkView.h"
#include "GPUView.h"
#include "SysInfoView.h"

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
    SummaryView() : BView("SummaryView", B_WILL_DRAW | B_PULSE_NEEDED) {
        SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

        fCpuGraph = new GraphView("cpu_summary_graph", (rgb_color){80, 255, 80, 255});
        fMemGraph = new GraphView("mem_summary_graph", (rgb_color){80, 80, 255, 255});
        fNetGraph = new GraphView("net_summary_graph", (rgb_color){255, 80, 80, 255});

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
        // TODO: Get real data
        fCpuGraph->AddSample(rand() % 100);
        fMemGraph->AddSample(rand() % 100);
        fNetGraph->AddSample(rand() % 100);
    }

private:
    GraphView* fCpuGraph;
    GraphView* fMemGraph;
    GraphView* fNetGraph;
};

// Performance Tab - combines CPU, Memory, Disk, Network, GPU monitoring
class PerformanceView : public BView {
public:
    PerformanceView() : BView("PerformanceView", B_WILL_DRAW) {
        SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

        BSplitView* splitView = new BSplitView(B_HORIZONTAL, B_USE_DEFAULT_SPACING);
        splitView->SetInsets(B_USE_DEFAULT_SPACING);

        BView* leftPane = new SummaryView();

        BTabView* tabView = new BTabView("tab_view", B_WIDTH_FROM_WIDEST);

        BView* cpuTab = new CPUView();
        BView* memTab = new MemView(Bounds());
        BView* netTab = new NetworkView(Bounds());
        BView* diskTab = new DiskView(Bounds());
        BView* gpuTab = new GPUView(Bounds());

        tabView->AddTab(cpuTab, new BTab("CPU"));
        tabView->AddTab(memTab, new BTab("Memory"));
        tabView->AddTab(netTab, new BTab("Network"));
        tabView->AddTab(diskTab, new BTab("Disk"));
        tabView->AddTab(gpuTab, new BTab("GPU"));

        BView* processTab = new ProcessView(Bounds());
        tabView->AddTab(processTab, new BTab("Processes"));

        splitView->AddChild(leftPane);
        splitView->AddChild(tabView);

        splitView->SetItemWeight(0, 0.25f);
        splitView->SetItemWeight(1, 0.75f);

        BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
            .Add(splitView)
            .End();
    }
};

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
