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
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Catalog.h>
#include <MenuBar.h>
#include <Menu.h>
#include <MenuItem.h>
#include <StringView.h>
#include <String.h>
#include <Font.h>
#include <Rect.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SysMonTaskApp"

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

const uint32 MSG_ABOUT_REQUESTED = 'abou';

class AboutWindow : public BWindow {
public:
    AboutWindow()
        : BWindow(BRect(0, 0, 300, 200), B_TRANSLATE("About SysMonTask"),
                  B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE)
    {
        BStringView* titleView = new BStringView("title", "SysMonTask");
        BFont titleFont(be_bold_font);
        titleFont.SetSize(titleFont.Size() * 1.5);
        titleView->SetFont(&titleFont);
        titleView->SetAlignment(B_ALIGN_CENTER);

        BStringView* descView = new BStringView("desc", B_TRANSLATE("A comprehensive system monitor for Haiku."));
        descView->SetAlignment(B_ALIGN_CENTER);

        BStringView* copyView = new BStringView("copyright", "Copyright 2023 Haiku Archives");
        copyView->SetAlignment(B_ALIGN_CENTER);

        BButton* okButton = new BButton("ok", "OK", new BMessage(B_QUIT_REQUESTED));

        BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
            .SetInsets(B_USE_DEFAULT_SPACING)
            .Add(titleView)
            .AddStrut(B_USE_DEFAULT_SPACING)
            .Add(descView)
            .Add(copyView)
            .AddStrut(B_USE_DEFAULT_SPACING)
            .Add(okButton)
            .End();

        CenterOnScreen();
    }
};

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

        fCpuGraph = new ActivityGraphView("cpu_summary_graph", {0, 0, 0, 0}, B_SUCCESS_COLOR);
        fMemGraph = new ActivityGraphView("mem_summary_graph", {0, 0, 0, 0}, B_MENU_SELECTION_BACKGROUND_COLOR);
        fNetGraph = new ActivityGraphView("net_summary_graph", {0, 0, 0, 0}, B_FAILURE_COLOR);

        BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
            .SetInsets(B_USE_DEFAULT_SPACING)
            .Add(new BStringView("cpu_label", B_TRANSLATE("CPU Usage")))
            .Add(fCpuGraph)
            .Add(new BStringView("mem_label", B_TRANSLATE("Memory Usage")))
            .Add(fMemGraph)
            .Add(new BStringView("net_label", B_TRANSLATE("Network Usage")))
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
        fMemView = new MemView();
        fNetworkView = new NetworkView();
        BView* diskTab = new DiskView();
        BView* gpuTab = new GPUView();

        tabView->AddTab(fCPUView);
        tabView->TabAt(0)->SetLabel(B_TRANSLATE("CPU"));
        tabView->AddTab(fMemView);
        tabView->TabAt(1)->SetLabel(B_TRANSLATE("Memory"));
        tabView->AddTab(fNetworkView);
        tabView->TabAt(2)->SetLabel(B_TRANSLATE("Network"));
        tabView->AddTab(diskTab);
        tabView->TabAt(3)->SetLabel(B_TRANSLATE("Disk"));
        tabView->AddTab(gpuTab);
        tabView->TabAt(4)->SetLabel(B_TRANSLATE("GPU"));

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

    void SaveSettings();
    void LoadSettings();

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
    : BWindow(frame, B_TRANSLATE("SysMonTask - Haiku System Monitor"), B_TITLED_WINDOW,
              B_QUIT_ON_WINDOW_CLOSE | B_AUTO_UPDATE_SIZE_LIMITS),
      fCurrentViewIndex(0) {

    // Create Menu Bar
    BMenuBar* menuBar = new BMenuBar("MenuBar");
    BMenu* appMenu = new BMenu(B_TRANSLATE("App"));
    BMenuItem* aboutItem = new BMenuItem(B_TRANSLATE("About"), new BMessage(MSG_ABOUT_REQUESTED));
    appMenu->AddItem(aboutItem);
    appMenu->AddSeparatorItem();
    appMenu->AddItem(new BMenuItem(B_TRANSLATE("Quit"), new BMessage(B_QUIT_REQUESTED), 'Q'));
    menuBar->AddItem(appMenu);

    // Create button bar
    fPerformanceButton = new BButton("Performance", B_TRANSLATE("Performance"), new BMessage(MSG_SWITCH_TO_PERFORMANCE));
    fProcessesButton = new BButton("Processes", B_TRANSLATE("Processes"), new BMessage(MSG_SWITCH_TO_PROCESSES));
    fSystemButton = new BButton("System", B_TRANSLATE("System"), new BMessage(MSG_SWITCH_TO_SYSTEM));
    
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
    fProcessView = new ProcessView();
    fSysInfoView = new SysInfoView();
    
    // Add views to card layout
    fCardLayout->AddView(fPerformanceView);
    fCardLayout->AddView(fProcessView);
    fCardLayout->AddView(fSysInfoView);
    
    // Set initial view
    fCardLayout->SetVisibleItem((int32)0);
    
    // Set up main window layout
    BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
        .Add(menuBar)
        .AddGroup(B_VERTICAL, 0)
            .SetInsets(5)
            .AddGroup(B_HORIZONTAL, 5)
                .Add(fPerformanceButton)
                .Add(fProcessesButton)
                .Add(fSystemButton)
                .AddGlue()
            .End()
            .Add(fCardLayout)
        .End()
    .End();
    
    // Configure window
    SetSizeLimits(800, 2000, 600, 1500);
    
    // Set pulse rate for real-time updates
    SetPulseRate(1000000); // 1 second pulse
    
    // Center window on screen
    CenterOnScreen();

    LoadSettings();
}

bool MainWindow::QuitRequested() {
    SaveSettings();
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
        case MSG_ABOUT_REQUESTED:
            {
                AboutWindow* about = new AboutWindow();
                about->Show();
            }
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

void MainWindow::SaveSettings() {
    BPath path;
    if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
        path.Append("SysMonTask_settings");
        BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
        if (file.InitCheck() == B_OK) {
            BMessage settings;
            if (fProcessView)
                fProcessView->SaveState(settings);
            settings.Flatten(&file);
        }
    }
}

void MainWindow::LoadSettings() {
    BPath path;
    if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
        path.Append("SysMonTask_settings");
        BFile file(path.Path(), B_READ_ONLY);
        if (file.InitCheck() == B_OK) {
            BMessage settings;
            if (settings.Unflatten(&file) == B_OK) {
                if (fProcessView)
                    fProcessView->LoadState(settings);
            }
        }
    }
}

SysMonTaskApp::SysMonTaskApp()
    : BApplication("application/x-vnd.HaikuSysMonTask") {
    // Constructor
}

void SysMonTaskApp::ReadyToRun() {
    BRect windowRect(100, 100, 1420, 667);
    mainWindow = new MainWindow(windowRect);
    mainWindow->Show();
}

int main() {
    SysMonTaskApp app;
    app.Run();
    return 0;
}
