#include "SystemSummaryView.h"
#include "Utils.h"
#include <kernel/OS.h>
#include <stdio.h>
#include <time.h>
#include <TextView.h>
#include <String.h>
#include <Alignment.h>
#include <SpaceLayoutItem.h>
#include <Font.h>
#include <LayoutBuilder.h>
#include <Directory.h>
#include <Path.h>
#include <Box.h>
#include <ScrollView.h>
#include <GridLayout.h>
#include <GroupLayout.h>
#include <Entry.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/utsname.h>
#include <Catalog.h>
#include <vector>
#include <cstring>
#include <NetworkRoster.h>
#include <NetworkInterface.h>
#include <NetworkAddress.h>
#include <Locale.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SystemSummaryView"

#include <InterfaceDefs.h>

static const uint32 kMsgUpdateInfo = 'UPDT';

SystemSummaryView::SystemSummaryView()
	: BView("SystemSummaryView", B_WILL_DRAW | B_PULSE_NEEDED),
	  fInfoTextView(NULL),
	  fLoadThread(-1),
	  fThreadRunning(false)
{
	SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR);
	CreateLayout();
}

SystemSummaryView::~SystemSummaryView()
{
	// Child views are automatically deleted
	if (fLoadThread >= 0) {
		wait_for_thread(fLoadThread, NULL);
	}
}

void SystemSummaryView::CreateLayout()
{
	fInfoTextView = new BTextView("info_text_view");
	fInfoTextView->SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR);
	fInfoTextView->SetStylable(true);
	fInfoTextView->MakeEditable(false);
	fInfoTextView->SetWordWrap(false);
	fInfoTextView->SetFontAndColor(be_fixed_font);

	BGroupView* groupView = new BGroupView(B_HORIZONTAL, B_USE_DEFAULT_SPACING);
	BLayoutBuilder::Group<>(groupView)
		.Add(fInfoTextView)
		.AddGlue()
	.End();

	BScrollView* scrollView = new BScrollView("sysInfoScroller", groupView,
		0, true, true, B_PLAIN_BORDER);
	scrollView->SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT));

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(scrollView)
	.End();
}


void SystemSummaryView::AttachedToWindow()
{
	BView::AttachedToWindow();
	_StartLoadThread();
}

void SystemSummaryView::Show()
{
	BView::Show();
	_StartLoadThread();
}

void SystemSummaryView::Pulse()
{
	if (!IsHidden()) {
		_StartLoadThread();
	}
}

void SystemSummaryView::_StartLoadThread()
{
	// Use atomic flag: prevents a new thread if one is already running,
	// even if the previous thread's ID has been recycled.
	bool expected = false;
	if (!fThreadRunning.compare_exchange_strong(expected, true))
		return;

	BMessenger* messenger = new BMessenger(this);
	fLoadThread = spawn_thread(_LoadDataThread, "sysinfo_loader",
		B_NORMAL_PRIORITY, messenger);
	if (fLoadThread >= 0) {
		resume_thread(fLoadThread);
	} else {
		delete messenger;
		fThreadRunning = false;
	}
}

void SystemSummaryView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgUpdateInfo: {
			fLoadThread = -1;
			fThreadRunning = false;

			_UpdateSystemInfo(message);
			break;
		}
		default:
			BView::MessageReceived(message);
	}
}

void SystemSummaryView::_UpdateSystemInfo(BMessage* message)
{
	// Info Section
	// Construct the information string field by field
	// Note: Colors are applied after setting the text
	BString infoText;

	// Build string first
	BString userHost = message->FindString("user_host");
	BString separator;
	separator.Append('-', userHost.Length());

	infoText << userHost << "\n" << separator << "\n";

	// Order from screenshot
	auto addInfoLine = [&](const char* key, const char* field) {
		BString localizedKey = B_TRANSLATE(key);
		infoText << localizedKey << ": " << message->FindString(field) << "\n";
	};

	addInfoLine("OS", "os");
	addInfoLine("Kernel", "kernel");
	addInfoLine("Uptime", "uptime");
	addInfoLine("Packages", "packages");
	addInfoLine("Shell", "shell");
	addInfoLine("Display", "display");
	addInfoLine("DE", "de");
	addInfoLine("WM", "wm");
	addInfoLine("Font", "font");
	addInfoLine("CPU", "cpu");
	addInfoLine("GPU", "gpu");
	addInfoLine("Memory", "memory");
	addInfoLine("Swap", "swap");
	addInfoLine("Disk", "disk");
	addInfoLine("Local IP", "ip");
	if (message->HasString("battery"))
		addInfoLine("Battery", "battery");
	addInfoLine("Locale", "locale");

	// Add Color Blocks at bottom
	infoText << "\n";
	// We will render blocks as full block chars
	BString blocks = "███ ███ ███ ███ ███ ███";
	infoText << blocks;

	fInfoTextView->SetText(infoText.String());

	// Apply Colors
	rgb_color userColor = {0, 0, 0, 255}; // Black
	rgb_color keyColor = {255, 100, 100, 255}; // Salmon/Red
	rgb_color sepColor = {200, 200, 200, 255}; // Grey

	BFont userFont(be_fixed_font);
	userFont.SetFace(B_BOLD_FACE);
	userFont.SetSize(userFont.Size() + 1);

	// 1. User@Host (Black)
	int32 pos = 0;
	int32 len = userHost.Length();
	fInfoTextView->SetFontAndColor(pos, pos + len, &userFont, B_FONT_ALL, &userColor);
	pos += len + 1; // newline

	// 2. Separator (Grey)
	len = separator.Length();
	fInfoTextView->SetFontAndColor(pos, pos + len, NULL, 0, &sepColor);
	pos += len + 1; // newline

	BFont keyFont(be_fixed_font);
	keyFont.SetFace(B_BOLD_FACE);
	keyFont.SetSize(keyFont.Size() + 2);

	// 3. Key: Value lines
	const char* keys[] = {
		"OS", "Kernel", "Uptime", "Packages", "Shell", "Display", "DE", "WM",
		"Font", "CPU", "GPU", "Memory", "Swap", "Disk", "Local IP", "Battery", "Locale", NULL
	};

	BString currentText = fInfoTextView->Text();
	for (int i=0; keys[i]; i++) {
		BString keyStr = B_TRANSLATE(keys[i]);
		keyStr << ":";
		int32 keyStart = currentText.FindFirst(keyStr, pos);
		if (keyStart >= 0) {
			fInfoTextView->SetFontAndColor(keyStart, keyStart + keyStr.Length(), &keyFont, B_FONT_ALL, &keyColor);
			pos = keyStart + keyStr.Length();
		}
	}

	// 4. Color Blocks (Manual coloring of the last line)
	// "███ ███ ███ ███ ███ ███"
	//  012 345 678 901 234 567
	//  Blk Red Grn Yel Blu Mag Cyn Wht ...
	int32 blockStart = currentText.FindFirst("███");
	if (blockStart >= 0) {
		rgb_color c1 = {0, 0, 0, 255};	   // Black
		rgb_color c2 = {255, 0, 0, 255};	 // Red
		rgb_color c3 = {0, 255, 0, 255};	 // Green
		rgb_color c4 = {255, 255, 0, 255};   // Yellow
		rgb_color c5 = {0, 0, 255, 255};	 // Blue
		rgb_color c6 = {255, 0, 255, 255};   // Magenta

		auto colorBlock = [&](int index, rgb_color c) {
			 // "███" is 9 bytes in UTF-8. " " is 1 byte.
			 // Stride is 10 bytes (9 + 1). Block length is 9.
			 int32 offset = index * 10;
			 fInfoTextView->SetFontAndColor(blockStart + offset, blockStart + offset + 9, NULL, 0, &c);
		};
		colorBlock(0, c1);
		colorBlock(1, c2);
		colorBlock(2, c3);
		colorBlock(3, c4);
		colorBlock(4, c5);
		colorBlock(5, c6);
	}
}

int32 SystemSummaryView::_LoadDataThread(void* data) {
	BMessenger* messenger = static_cast<BMessenger*>(data);
	if (!messenger) return B_BAD_VALUE;

	BMessage reply(kMsgUpdateInfo);
	system_info sysInfo;

	// 1. User@Host
	struct passwd* pw = getpwuid(getuid());
	char hostname[256];
	if (gethostname(hostname, sizeof(hostname)) != 0)
		strlcpy(hostname, B_TRANSLATE("unknown"), sizeof(hostname));
	else
		hostname[sizeof(hostname) - 1] = '\0';

	BString userHost;
	userHost << (pw && pw->pw_name ? pw->pw_name : "user") << "@" << hostname;
	reply.AddString("user_host", userHost);

	// 2. OS
	reply.AddString("os", GetOSVersion());

	// 3. Kernel
	struct utsname u;
	uname(&u);
	BString kernel;
	kernel << u.sysname << " " << u.release;
	reply.AddString("kernel", kernel);

	// 4. Uptime
	reply.AddString("uptime", ::FormatUptime(system_time()));

	// 5. Packages
	BString packages;
	GetPackageCount(packages);
	reply.AddString("packages", packages);

	// 6. Shell
	const char* shellEnv = getenv("SHELL");
	BString shell = shellEnv ? shellEnv : "/bin/sh";
	BPath shellPath(shell.String());
	if (shellPath.InitCheck() == B_OK) shell = shellPath.Leaf();
	reply.AddString("shell", shell);

	// 7. Display
	reply.AddString("display", GetDisplayInfo());

	// 8. DE / WM
	reply.AddString("de", B_TRANSLATE("Application Kit"));
	reply.AddString("wm", B_TRANSLATE("Application Server"));

	// 9. Font
	font_family family;
	font_style style;
	be_plain_font->GetFamilyAndStyle(&family, &style);
	BString font;
	font << family << " " << style << " (" << static_cast<int>(be_plain_font->Size()) << "pt)";
	reply.AddString("font", font);

	// 10. CPU
	reply.AddString("cpu", ::GetCPUBrandString());

	// 11. GPU
	reply.AddString("gpu", GetGPUInfo());

	// 12. Memory
	uint64 used, total, physical;
	GetMemoryUsage(used, total, physical);
	if (total > 0 && get_system_info(&sysInfo) == B_OK) {
		uint64 cached = GetCachedMemoryBytes(sysInfo);

		BString cachedStr;
		::FormatBytes(cachedStr, cached);

		BString memStr;
		int percent = static_cast<int>(100.0 * used / total);
		BString usedStr, totalStr;
		::FormatBytes(usedStr, used);
		::FormatBytes(totalStr, total);
		memStr.SetToFormat(B_TRANSLATE("%s / %s (%d%%), Cached: %s"),
			usedStr.String(), totalStr.String(), percent, cachedStr.String());
		reply.AddString("memory", memStr);

		uint64 swapUsed, swapTotal;
		::GetSwapUsage(swapUsed, swapTotal);

		BString swapStr;
		BString swapUsedStr, swapTotalStr;
		::FormatBytes(swapUsedStr, swapUsed);
		::FormatBytes(swapTotalStr, swapTotal);
		int swapPercent = swapTotal > 0 ? static_cast<int>(100.0 * swapUsed / swapTotal) : 0;
		swapStr.SetToFormat(B_TRANSLATE("%s / %s (%d%%)"),
			swapUsedStr.String(), swapTotalStr.String(), swapPercent);
		reply.AddString("swap", swapStr);
	}

	// 13. Disk (Root volume)
	reply.AddString("disk", GetRootDiskUsage());

	// 14. IP
	reply.AddString("ip", GetLocalIPAddress());

	// 15. Battery
	reply.AddString("battery", GetBatteryCapacity());

	// 16. Locale
	reply.AddString("locale", GetLocale());

	messenger->SendMessage(&reply);

	delete messenger;
	return B_OK;
}
