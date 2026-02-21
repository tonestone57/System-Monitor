#include <Application.h>
#include <Window.h>
#include <Alert.h>
#include <LayoutBuilder.h>
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
#include <Button.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SysMonTaskApp"

#include "ProcessView.h"
#include "SystemTab.h"
#include "PerformanceView.h"

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

		BButton* okButton = new BButton("ok", B_TRANSLATE("OK"), new BMessage(B_QUIT_REQUESTED));

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

#include "PerformanceView.h"

const uint32 MSG_REFRESH_SPEED_HIGH = 'rfsH';
const uint32 MSG_REFRESH_SPEED_NORMAL = 'rfsN';
const uint32 MSG_REFRESH_SPEED_LOW = 'rfsL';

class MainWindow : public BWindow {
public:
	MainWindow(BRect frame);
	virtual bool QuitRequested();
	virtual void MessageReceived(BMessage* message);

	void SaveSettings();
	void LoadSettings();

private:

	BTabView* fMainTabView;

	ProcessView* fProcessView;
	PerformanceView* fPerformanceView;
	SystemTab* fSystemTab;

	BMessenger fAboutWindow;
};

MainWindow::MainWindow(BRect frame)
	: BWindow(frame, B_TRANSLATE("SysMonTask - Haiku System Monitor"), B_TITLED_WINDOW,
			  B_QUIT_ON_WINDOW_CLOSE) {

	// Create Menu Bar
	BMenuBar* menuBar = new BMenuBar("MenuBar");
	BMenu* appMenu = new BMenu(B_TRANSLATE("App"));
	BMenuItem* aboutItem = new BMenuItem(B_TRANSLATE("About"), new BMessage(MSG_ABOUT_REQUESTED));
	appMenu->AddItem(aboutItem);
	appMenu->AddSeparatorItem();
	appMenu->AddItem(new BMenuItem(B_TRANSLATE("Quit"), new BMessage(B_QUIT_REQUESTED), 'Q'));
	menuBar->AddItem(appMenu);

	BMenu* viewMenu = new BMenu(B_TRANSLATE("View"));
	BMenu* speedMenu = new BMenu(B_TRANSLATE("Refresh Speed"));
	speedMenu->AddItem(new BMenuItem(B_TRANSLATE("High (0.5s)"), new BMessage(MSG_REFRESH_SPEED_HIGH)));
	speedMenu->AddItem(new BMenuItem(B_TRANSLATE("Normal (1s)"), new BMessage(MSG_REFRESH_SPEED_NORMAL)));
	speedMenu->AddItem(new BMenuItem(B_TRANSLATE("Low (2s)"), new BMessage(MSG_REFRESH_SPEED_LOW)));
	speedMenu->SetRadioMode(true);
	viewMenu->AddItem(speedMenu);
	menuBar->AddItem(viewMenu);

	// Create the three main views
	fPerformanceView = new PerformanceView();
	fProcessView = new ProcessView();
	fSystemTab = new SystemTab();

	// Create Main Tab View
	fMainTabView = new BTabView("main_tab_view");
	fMainTabView->AddTab(fPerformanceView);
	fMainTabView->TabAt(0)->SetLabel(B_TRANSLATE("Performance"));

	fMainTabView->AddTab(fProcessView);
	fMainTabView->TabAt(1)->SetLabel(B_TRANSLATE("Processes"));

	fMainTabView->AddTab(fSystemTab);
	fMainTabView->TabAt(2)->SetLabel(B_TRANSLATE("System"));

	// Set up main window layout
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(menuBar)
		.AddGroup(B_VERTICAL, 0)
			.SetInsets(B_USE_DEFAULT_SPACING)
			.Add(fMainTabView)
		.End()
	.End();

	// Configure window
	SetSizeLimits(800, B_SIZE_UNLIMITED, 600, B_SIZE_UNLIMITED);

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
		case MSG_ABOUT_REQUESTED:
			{
				if (fAboutWindow.IsValid()) {
					BWindow* window;
					if (fAboutWindow.Target(&window) == B_OK && window != NULL) {
						window->Activate(true);
						break;
					}
				}
				AboutWindow* about = new AboutWindow();
				about->Show();
				fAboutWindow = BMessenger(about);
			}
			break;

		case MSG_REFRESH_SPEED_HIGH:
			SetPulseRate(500000);
			if (fProcessView) fProcessView->SetRefreshInterval(500000);
			if (fPerformanceView) fPerformanceView->SetRefreshInterval(500000);
			break;
		case MSG_REFRESH_SPEED_NORMAL:
			SetPulseRate(1000000);
			if (fProcessView) fProcessView->SetRefreshInterval(1000000);
			if (fPerformanceView) fPerformanceView->SetRefreshInterval(1000000);
			break;
		case MSG_REFRESH_SPEED_LOW:
			SetPulseRate(2000000);
			if (fProcessView) fProcessView->SetRefreshInterval(2000000);
			if (fPerformanceView) fPerformanceView->SetRefreshInterval(2000000);
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
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
			if (fPerformanceView)
				fPerformanceView->SaveState(settings);

			settings.AddRect("window_frame", Frame());
			settings.AddInt64("pulse_rate", PulseRate());

			if (fMainTabView)
				settings.AddInt32("active_tab", fMainTabView->Selection());

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
				BRect frame;
				if (settings.FindRect("window_frame", &frame) == B_OK) {
					MoveTo(frame.LeftTop());
					ResizeTo(frame.Width(), frame.Height());
				}

				if (fProcessView)
					fProcessView->LoadState(settings);
				if (fPerformanceView)
					fPerformanceView->LoadState(settings);

				bigtime_t rate;
				if (settings.FindInt64("pulse_rate", &rate) == B_OK) {
					SetPulseRate(rate);
					if (fProcessView) fProcessView->SetRefreshInterval(rate);
					if (fPerformanceView) fPerformanceView->SetRefreshInterval(rate);
				}

				int32 activeTab;
				if (fMainTabView && settings.FindInt32("active_tab", &activeTab) == B_OK) {
					fMainTabView->Select(activeTab);
				}
			}
		}
	}

	// Update Menu Selection based on current PulseRate
	bigtime_t rate = PulseRate();
	uint32 command = MSG_REFRESH_SPEED_NORMAL;
	if (rate <= 500000) command = MSG_REFRESH_SPEED_HIGH;
	else if (rate >= 2000000) command = MSG_REFRESH_SPEED_LOW;

	// Fallback to searching all children if KeyMenuBar is not yet set
	BMenuBar* menuBar = KeyMenuBar();
	if (!menuBar) {
		menuBar = dynamic_cast<BMenuBar*>(FindView("MenuBar"));
	}

	if (menuBar) {
		BMenuItem* item = menuBar->FindItem(command);
		if (item) item->SetMarked(true);
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
