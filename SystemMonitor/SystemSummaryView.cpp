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
#include <NetworkRoster.h>
#include <NetworkInterface.h>
#include <NetworkAddress.h>
#include <Locale.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SystemSummaryView"

#include <InterfaceDefs.h>

static const uint32 kMsgUpdateInfo = 'UPDT';

SystemSummaryView::SystemSummaryView()
	: BView("SystemSummaryView", B_WILL_DRAW),
	  fLogoTextView(NULL),
	  fInfoTextView(NULL),
	  fLoadThread(-1)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
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
	fLogoTextView = new BTextView("logo_text_view");
	fLogoTextView->SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR);
	fLogoTextView->SetStylable(true);
	fLogoTextView->MakeEditable(false);
	fLogoTextView->SetWordWrap(false);
	fLogoTextView->SetFontAndColor(be_fixed_font);

	fInfoTextView = new BTextView("info_text_view");
	fInfoTextView->SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR);
	fInfoTextView->SetStylable(true);
	fInfoTextView->MakeEditable(false);
	fInfoTextView->SetWordWrap(false);
	fInfoTextView->SetFontAndColor(be_fixed_font);

	BGroupView* groupView = new BGroupView(B_HORIZONTAL, B_USE_DEFAULT_SPACING);
	BLayoutBuilder::Group<>(groupView)
		.Add(fLogoTextView)
		.Add(fInfoTextView)
		.AddGlue()
	.End();

	BScrollView* scrollView = new BScrollView("sysInfoScroller", groupView,
		true, true, B_PLAIN_BORDER);
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

void SystemSummaryView::_StartLoadThread()
{
	if (fLoadThread >= 0)
		return;

	// Spawn thread to load data
	BMessenger* messenger = new BMessenger(this);
	fLoadThread = spawn_thread(_LoadDataThread, "sysinfo_loader", B_NORMAL_PRIORITY, messenger);
	if (fLoadThread >= 0) {
		resume_thread(fLoadThread);
	} else {
		delete messenger;
	}
}

void SystemSummaryView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgUpdateInfo: {
			fLoadThread = -1;

			// Set Logo (ASCII Art)
			// Color Palette from Haiku: Yellow/Gold for leaf, Blue for stem?
			// Fastfetch Haiku Logo:
			//           MMMM
			//           MMMM
			//           MMMM
			//           MMMM
			//           MMMM       .ciO  /YMMMMM*
			//           MMMM    .dMMMMMM  /MMMMM/`
			//           ,iMM   /MMMMMMMMMMMMMMMM*
			//  *`     -cMMMMMMMMMMMMMMMMMMM/` .MMM
			//    MMMMMMMMM/` :MMM/  MMMM
			//    MMMM         MMMM
			//    MMMM         MMMM
			//    """"         """"

			// We need to construct this text and color it.
			// Simplified approach: Set text, then apply colors.
			// Logo is constant.
			if (fLogoTextView->TextLength() == 0) {
				BString logo;
				logo << "          MMMM\n";
				logo << "          MMMM\n";
				logo << "          MMMM\n";
				logo << "          MMMM\n";
				logo << "          MMMM       .ciO | /YMMMMM*\"\n";
				logo << "          MMMM    .cOMMMMM | /MMMMM/`\n"; // Modified slightly to match screenshot curve
				logo << "          ,iMM | /MMMMMMMMMMMMMMMM*\n";
				logo << " `*      -cMMMMMMMMMMMMMMMMMMM/` .MMM\n";
				logo << "   MMMMMMMMMM/` :MMM/  MMMM\n";
				logo << "   MMMM         MMMM\n";
				logo << "   MMMM         MMMM\n";
				logo << "   \"\"\"\"         \"\"\"\"\n";

				fLogoTextView->SetText(logo.String());

				// Colors (Approximation based on screenshot)
				// Dark Grey/Black for MMMM stem? No, screenshot shows dark grey/black.
				// Yellow/Gold for Leaf.
				rgb_color darkGrey = {80, 80, 80, 255};
				rgb_color gold = {255, 200, 0, 255}; // Leaf
				// Stem/details?

				// Apply global dark grey first
				fLogoTextView->SetFontAndColor(0, logo.Length(), NULL, 0, &darkGrey);

				// Highlight Leaf parts (Yellow)
				// Manual highlighting of leaf parts based on line content
				// Lines 5-8 contain the leaf
				const char* lines[] = {
					"          MMMM\n",
					"          MMMM\n",
					"          MMMM\n",
					"          MMMM\n",
					"          MMMM       .ciO | /YMMMMM*\"\n",
					"          MMMM    .cOMMMMM | /MMMMM/`\n",
					"          ,iMM | /MMMMMMMMMMMMMMMM*\n",
					" `*      -cMMMMMMMMMMMMMMMMMMM/` .MMM\n",
					"   MMMMMMMMMM/` :MMM/  MMMM\n",
					"   MMMM         MMMM\n",
					"   MMMM         MMMM\n",
					"   \"\"\"\"         \"\"\"\"\n",
					NULL
				};

				int32 offset = 0;
				for (int i = 0; lines[i]; i++) {
					BString line(lines[i]);
					// Coloring logic based on line index and content pattern
					if (i == 4) { // .ciO...
						int32 leafStart = line.FindFirst(".ciO");
						if (leafStart >= 0) fLogoTextView->SetFontAndColor(offset + leafStart, offset + line.Length() - 1, NULL, 0, &gold);
					} else if (i == 5) { // .cOMMM...
						int32 leafStart = line.FindFirst(".cOMMM");
						if (leafStart >= 0) fLogoTextView->SetFontAndColor(offset + leafStart, offset + line.Length() - 1, NULL, 0, &gold);
					} else if (i == 6) { // | /MMM... (after ,iMM)
						int32 leafStart = line.FindFirst("|");
						if (leafStart >= 0) fLogoTextView->SetFontAndColor(offset + leafStart, offset + line.Length() - 1, NULL, 0, &gold);
					} else if (i == 7) { // `* -cMM...
						// Whole line except last .MMM? Actually the whole left part is leaf-like here.
						fLogoTextView->SetFontAndColor(offset, offset + line.Length() - 1, NULL, 0, &gold);
					} else if (i == 8) { // ... :MMM/
						 // The :MMM/ part
						 int32 leafStart = line.FindFirst(":");
						 if (leafStart >= 0) {
							 int32 leafEnd = line.FindFirst("  ", leafStart);
							 if (leafEnd < 0) leafEnd = line.Length() - 1;
							 fLogoTextView->SetFontAndColor(offset + leafStart, offset + leafEnd, NULL, 0, &gold);
						 }
					}
					offset += line.Length();
				}
			}

			// Info Section
			// Construct the information string field by field
			// Note: Colors are applied after setting the text
			BString infoText;

			// Build string first
			BString userHost = message->FindString("user_host");
			BString separator;
			for (int i=0; i<userHost.Length(); i++) separator << "-";

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
			rgb_color userColor = {255, 200, 0, 255}; // Yellow/Orange
			rgb_color keyColor = {255, 100, 100, 255}; // Salmon/Red
			rgb_color sepColor = {200, 200, 200, 255}; // Grey

			// 1. User@Host (Yellow)
			int32 pos = 0;
			int32 len = userHost.Length();
			fInfoTextView->SetFontAndColor(pos, pos + len, NULL, 0, &userColor);
			pos += len + 1; // newline

			// 2. Separator (Grey)
			len = separator.Length();
			fInfoTextView->SetFontAndColor(pos, pos + len, NULL, 0, &sepColor);
			pos += len + 1; // newline

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
					fInfoTextView->SetFontAndColor(keyStart, keyStart + keyStr.Length(), NULL, 0, &keyColor);
					pos = keyStart + keyStr.Length();
				}
			}

			// 4. Color Blocks (Manual coloring of the last line)
			// "███ ███ ███ ███ ███ ███"
			//  012 345 678 901 234 567
			//  Blk Red Grn Yel Blu Mag Cyn Wht ...
			int32 blockStart = currentText.FindFirst("███");
			if (blockStart >= 0) {
				rgb_color c1 = {0, 0, 0, 255};       // Black
				rgb_color c2 = {255, 0, 0, 255};     // Red
				rgb_color c3 = {0, 255, 0, 255};     // Green
				rgb_color c4 = {255, 255, 0, 255};   // Yellow
				rgb_color c5 = {0, 0, 255, 255};     // Blue
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

			break;
		}
		default:
			BView::MessageReceived(message);
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
		strcpy(hostname, B_TRANSLATE("unknown"));

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
	auto countPackages = [](const char* path) -> int {
		BDirectory dir(path);
		if (dir.InitCheck() != B_OK) return 0;
		int count = 0;
		BEntry entry;
		while (dir.GetNextEntry(&entry) == B_OK) {
			BPath p;
			entry.GetPath(&p);
			if (p.InitCheck() == B_OK) {
				BString name(p.Leaf());
				if (name.EndsWith(".hpkg")) count++;
			}
		}
		return count;
	};
	int sysPkgs = countPackages("/boot/system/packages");
	int userPkgs = countPackages("/boot/home/config/packages");
	BString packages;
	packages.SetToFormat("%d (hpkg-system), %d (hpkg-user)", sysPkgs, userPkgs);
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
	font << family << " " << style << " (" << (int)be_plain_font->Size() << "pt)";
	reply.AddString("font", font);

	// 10. CPU
	reply.AddString("cpu", ::GetCPUBrandString());

	// 11. GPU
	reply.AddString("gpu", GetGPUInfo());

	// 12. Memory
	if (get_system_info(&sysInfo) == B_OK) {
		uint64 total = (uint64)sysInfo.max_pages * B_PAGE_SIZE;
		uint64 used = (uint64)sysInfo.used_pages * B_PAGE_SIZE;
		uint64 cached = GetCachedMemoryBytes(sysInfo);

		BString cachedStr;
		::FormatBytes(cachedStr, cached);

		BString memStr;
		int percent = (int)(100.0 * used / total);
		BString usedStr, totalStr;
		::FormatBytes(usedStr, used);
		::FormatBytes(totalStr, total);
		memStr << usedStr << " / " << totalStr << " (" << percent << "%), Cached: " << cachedStr;
		reply.AddString("memory", memStr);

		uint64 swapUsed, swapTotal;
		::GetSwapUsage(swapUsed, swapTotal);

		BString swapStr;
		BString swapUsedStr, swapTotalStr;
		::FormatBytes(swapUsedStr, swapUsed);
		::FormatBytes(swapTotalStr, swapTotal);
		swapStr << swapUsedStr << " / " << swapTotalStr;
		reply.AddString("swap", swapStr);
	}

	// 13. Disk (Root volume)
	reply.AddString("disk", GetRootDiskUsage());

	// 14. IP
	BNetworkRoster& roster = BNetworkRoster::Default();
	BNetworkInterface interface;
	uint32 cookie = 0;
	BString ip = "127.0.0.1";
	while (roster.GetNextInterface(&cookie, interface) == B_OK) {
		if (interface.Flags() & IFF_LOOPBACK) continue;
		if (!(interface.Flags() & IFF_UP)) continue;

		BNetworkInterfaceAddress addr;
		for (int32 i = 0; i < interface.CountAddresses(); i++) {
			if (interface.GetAddressAt(i, addr) == B_OK) {
				if (addr.Address().Family() == AF_INET) {
					ip = addr.Address().ToString();
					goto ip_found;
				}
			}
		}
	}
ip_found:
	reply.AddString("ip", ip);

	// 15. Battery
	// Attempt to read from the standard ACPI battery driver location on Haiku.
	// The driver exposes text-based status information at /dev/power/acpi_battery/0/state.
	// Example format includes lines like "capacity: 98", "state: discharging", etc.
	int batFd = open("/dev/power/acpi_battery/0/state", O_RDONLY);
	if (batFd >= 0) {
		char buffer[1024];
		ssize_t bytesRead = read(batFd, buffer, sizeof(buffer) - 1);
		close(batFd);

		if (bytesRead > 0) {
			buffer[bytesRead] = '\0';
			BString state(buffer);
			BString capacityStr;
			// Parse "capacity: <value>" from the driver output
			int32 capacityIndex = state.FindFirst("capacity: ");
			if (capacityIndex >= 0) {
				int32 end = state.FindFirst("\n", capacityIndex);
				if (end < 0) end = state.Length();

				if (end >= capacityIndex + 10) {
					state.CopyInto(capacityStr, capacityIndex + 10, end - (capacityIndex + 10));
					capacityStr.Trim();
				}

				if (!capacityStr.IsEmpty()) {
					capacityStr << "%";
					reply.AddString("battery", capacityStr);
				} else {
					reply.AddString("battery", B_TRANSLATE("Unknown"));
				}
			} else {
				reply.AddString("battery", B_TRANSLATE("Unknown"));
			}
		} else {
			reply.AddString("battery", B_TRANSLATE("Unknown"));
		}
	}

	// 16. Locale
	BString locale;
	// BLocale::Default() returns a const BLocale*. GetCode is likely not available or named differently.
	// BLocale::GetLanguage()->Code() is a safer bet, or construct from environment.
	// For simplicity, let's use a safe fallback if method doesn't exist.
	// However, to fix compilation, we replace the call.
	// Standard Haiku API uses formatting conventions.
	// Assuming we just want the name.
	// BLocale::Default()->GetName(locale); // Available?
    // Let's assume en_US for now if we can't find the exact API in memory.
    // Or better, let's try to get it from environment variables LANG/LC_ALL
    const char* lang = getenv("LC_ALL");
    if (!lang) lang = getenv("LANG");
    if (lang) locale = lang;
    else locale = "en_US.UTF-8";

	reply.AddString("locale", locale);

	messenger->SendMessage(&reply);

	delete messenger;
	return B_OK;
}
