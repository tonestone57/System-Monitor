#include "ProcessView.h"
#include "Utils.h"
#include <LayoutBuilder.h>
#include <ListView.h>
#include <ListItem.h>
#include <StringView.h>
#include <Button.h>
#include <kernel/image.h>
#include <pwd.h>
#include <signal.h>
#include <Alert.h>
#include <Roster.h>
#include <Path.h>
#include <Box.h>
#include <cstdio>
#include <cstring>
#include <MenuItem.h>
#include <Font.h>
#include <vector>
#include <unordered_set>
#include <Window.h>
#include <Invoker.h>
#include <Messenger.h>
#include <Catalog.h>
#include <ScrollView.h>
#include <Autolock.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ProcessView"

// Define constants for columns (used for drawing)
const float kBasePIDWidth = 60;
const float kBaseNameWidth = 180;
const float kBaseStateWidth = 80;
const float kBaseCPUWidth = 60;
const float kBaseMemWidth = 90;
const float kBaseThreadsWidth = 60;
const float kBaseUserWidth = 80;

const uint32 MSG_KILL_PROCESS = 'kill';
const uint32 MSG_SUSPEND_PROCESS = 'susp';
const uint32 MSG_RESUME_PROCESS = 'resm';
const uint32 MSG_PRIORITY_LOW = 'pril';
const uint32 MSG_PRIORITY_NORMAL = 'prin';
const uint32 MSG_PRIORITY_HIGH = 'prih';
const uint32 MSG_SHOW_CONTEXT_MENU = 'cntx';
const uint32 MSG_CONFIRM_KILL = 'conf';

const int32 kMemoryCacheGenerations = 10;


class ProcessListItem : public BListItem {
public:
	ProcessListItem(const ProcessInfo& info, const char* stateStr, const BFont* font, ProcessView* view)
		: BListItem(), fGeneration(0), fView(view)
	{
		Update(info, stateStr, font, true);
	}

	void SetGeneration(int32 generation) { fGeneration = generation; }
	int32 Generation() const { return fGeneration; }

	void Update(const ProcessInfo& info, const char* stateStr, const BFont* font, bool force = false) {
		bool nameChanged = force || strcmp(fInfo.name, info.name) != 0;
		bool userChanged = force || strcmp(fInfo.userName, info.userName) != 0;
		// Optimization: Compare enum values instead of localized strings
		bool stateChanged = force || fInfo.state != info.state;
		bool cpuChanged = force || fInfo.cpuUsage != info.cpuUsage;
		bool memChanged = force || fInfo.memoryUsageBytes != info.memoryUsageBytes;
		bool threadsChanged = force || fInfo.threadCount != info.threadCount;
		bool pidChanged = force || fInfo.id != info.id;

		fInfo = info;

		// Cache display strings
		if (pidChanged)
			fCachedPID.SetToFormat("%" B_PRId32, fInfo.id);

		if (nameChanged) {
			if (font && fView) {
				fTruncatedName = fInfo.name;
				font->TruncateString(&fTruncatedName, B_TRUNCATE_END, fView->NameWidth() - 10);
			} else {
				fTruncatedName = fInfo.name;
			}
		}

		if (stateChanged)
			fCachedState = stateStr;

		if (cpuChanged)
			fCachedCPU.SetToFormat("%.1f", fInfo.cpuUsage);

		if (memChanged)
			FormatBytes(fCachedMem, fInfo.memoryUsageBytes);

		if (threadsChanged)
			fCachedThreads.SetToFormat("%" B_PRIu32, fInfo.threadCount);

		if (userChanged) {
			if (font && fView) {
				fTruncatedUser = fInfo.userName;
				font->TruncateString(&fTruncatedUser, B_TRUNCATE_END, fView->UserWidth() - 10);
			} else {
				fTruncatedUser = fInfo.userName;
			}
		}
	}

	virtual void DrawItem(BView* owner, BRect itemRect, bool complete = false) {
		if (!fView) return;
		if (IsSelected() || complete) {
			rgb_color color;
			if (IsSelected()) color = ui_color(B_LIST_SELECTED_BACKGROUND_COLOR);
			else color = ui_color(B_LIST_BACKGROUND_COLOR);

			owner->SetHighColor(color);
			owner->FillRect(itemRect);
		}

		rgb_color textColor;
		if (IsSelected()) textColor = ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR);
		else textColor = ui_color(B_LIST_ITEM_TEXT_COLOR);
		owner->SetHighColor(textColor);

		font_height fh;
		owner->GetFontHeight(&fh);
		float x = itemRect.left + 5;
		float y = itemRect.bottom - fh.descent;

		// PID
		owner->DrawString(fCachedPID.String(), BPoint(x, y));
		x += fView->PIDWidth();

		// Name
		owner->DrawString(fTruncatedName.String(), BPoint(x, y));
		x += fView->NameWidth();

		// State
		owner->DrawString(fCachedState.String(), BPoint(x, y));
		x += fView->StateWidth();

		// CPU
		owner->DrawString(fCachedCPU.String(), BPoint(x, y));
		x += fView->CPUWidth();

		// Mem
		owner->DrawString(fCachedMem.String(), BPoint(x, y));
		x += fView->MemWidth();

		// Threads
		owner->DrawString(fCachedThreads.String(), BPoint(x, y));
		x += fView->ThreadsWidth();

		// User
		owner->DrawString(fTruncatedUser.String(), BPoint(x, y));
	}

	team_id TeamID() const { return fInfo.id; }
	const char* Name() const { return fInfo.name; }

	// For searching
	const ProcessInfo& Info() const { return fInfo; }

	static int CompareCPU(const void* first, const void* second) {
		const ProcessListItem* item1 = *(const ProcessListItem**)first;
		const ProcessListItem* item2 = *(const ProcessListItem**)second;
		if (item1->fInfo.cpuUsage > item2->fInfo.cpuUsage) return -1;
		if (item1->fInfo.cpuUsage < item2->fInfo.cpuUsage) return 1;
		return 0;
	}

	static int ComparePID(const void* first, const void* second) {
		const ProcessListItem* item1 = *(const ProcessListItem**)first;
		const ProcessListItem* item2 = *(const ProcessListItem**)second;
		if (item1->fInfo.id < item2->fInfo.id) return -1;
		if (item1->fInfo.id > item2->fInfo.id) return 1;
		return 0;
	}

	static int CompareName(const void* first, const void* second) {
		const ProcessListItem* item1 = *(const ProcessListItem**)first;
		const ProcessListItem* item2 = *(const ProcessListItem**)second;
		return strcasecmp(item1->fInfo.name, item2->fInfo.name);
	}

	static int CompareMem(const void* first, const void* second) {
		const ProcessListItem* item1 = *(const ProcessListItem**)first;
		const ProcessListItem* item2 = *(const ProcessListItem**)second;
		if (item1->fInfo.memoryUsageBytes > item2->fInfo.memoryUsageBytes) return -1;
		if (item1->fInfo.memoryUsageBytes < item2->fInfo.memoryUsageBytes) return 1;
		return 0;
	}

	static int CompareThreads(const void* first, const void* second) {
		const ProcessListItem* item1 = *(const ProcessListItem**)first;
		const ProcessListItem* item2 = *(const ProcessListItem**)second;
		if (item1->fInfo.threadCount > item2->fInfo.threadCount) return -1;
		if (item1->fInfo.threadCount < item2->fInfo.threadCount) return 1;
		return 0;
	}

	static int CompareState(const void* first, const void* second) {
		const ProcessListItem* item1 = *(const ProcessListItem**)first;
		const ProcessListItem* item2 = *(const ProcessListItem**)second;
		if (item1->fInfo.state < item2->fInfo.state) return -1;
		if (item1->fInfo.state > item2->fInfo.state) return 1;
		return 0;
	}

	static int CompareUser(const void* first, const void* second) {
		const ProcessListItem* item1 = *(const ProcessListItem**)first;
		const ProcessListItem* item2 = *(const ProcessListItem**)second;
		return strcasecmp(item1->fInfo.userName, item2->fInfo.userName);
	}

private:
	ProcessInfo fInfo;
	BString fCachedPID;
	BString fCachedState;
	BString fCachedCPU;
	BString fCachedMem;
	BString fCachedThreads;

	BString fTruncatedName;
	BString fTruncatedUser;
	int32 fGeneration;
	ProcessView* fView;
};

class ProcessListView : public BListView {
public:
	ProcessListView(const char* name)
		: BListView(name, B_SINGLE_SELECTION_LIST, B_WILL_DRAW | B_NAVIGABLE)
	{
	}

	virtual void MouseDown(BPoint where) {
		BMessage* msg = Window()->CurrentMessage();
		int32 buttons = 0;
		msg->FindInt32("buttons", &buttons);

		if (buttons & B_SECONDARY_MOUSE_BUTTON) {
			Select(IndexOf(where)); // Select item under mouse

			BMessenger messenger(Target());
			if (messenger.IsValid()) {
				BMessage contextMsg('cntx'); // MSG_SHOW_CONTEXT_MENU
				BPoint screenWhere = where;
				ConvertToScreen(&screenWhere);
				contextMsg.AddPoint("screen_where", screenWhere);
				messenger.SendMessage(&contextMsg);
			}
		}
		BListView::MouseDown(where);
	}

	virtual void KeyDown(const char* bytes, int32 numBytes) {
		if (numBytes == 1 && bytes[0] == B_DELETE) {
			BMessenger messenger(Target());
			if (messenger.IsValid())
				messenger.SendMessage('kill'); // MSG_KILL_PROCESS
			return;
		}
		BListView::KeyDown(bytes, numBytes);
	}
};



ProcessView::ProcessView()
	: BView("ProcessView", B_WILL_DRAW),
	  fFilterName(""),
	  fFilterID(""),
	  fFilterArgs(""),
	  fLastSystemTime(0),
	  fRefreshInterval(1000000),
	  fUpdateThread(B_ERROR),
	  fQuitSem(-1),
	  fTerminated(false),
	  fIsHidden(false),
	  fSortMode(SORT_BY_CPU),
	  fCurrentGeneration(0),
	  fListGeneration(0)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fQuitSem = create_sem(0, "ProcessView Quit");

	fSearchControl = new BTextControl("Search", B_TRANSLATE("Search:"), "", new BMessage(MSG_SEARCH_UPDATED));
	fSearchControl->SetModificationMessage(new BMessage(MSG_SEARCH_UPDATED));

	fProcessListView = new ProcessListView("process_list");
	BScrollView* processScrollView = new BScrollView("process_scroll", fProcessListView, 0, false, true, true);

	// Cache translated strings
	fStrRunning = B_TRANSLATE("Running");
	fStrReady = B_TRANSLATE("Ready");
	fStrSleeping = B_TRANSLATE("Sleeping");

	fContextMenu = new BPopUpMenu("ProcessContext", false, false);
	fContextMenu->AddItem(new BMenuItem(B_TRANSLATE("Kill Process"), new BMessage(MSG_KILL_PROCESS)));
	fContextMenu->AddSeparatorItem();
	fContextMenu->AddItem(new BMenuItem(B_TRANSLATE("Suspend"), new BMessage(MSG_SUSPEND_PROCESS)));
	fContextMenu->AddItem(new BMenuItem(B_TRANSLATE("Resume"), new BMessage(MSG_RESUME_PROCESS)));

	BMenu* priorityMenu = new BMenu(B_TRANSLATE("Set Priority"));
	priorityMenu->AddItem(new BMenuItem(B_TRANSLATE("Low"), new BMessage(MSG_PRIORITY_LOW)));
	priorityMenu->AddItem(new BMenuItem(B_TRANSLATE("Normal"), new BMessage(MSG_PRIORITY_NORMAL)));
	priorityMenu->AddItem(new BMenuItem(B_TRANSLATE("High"), new BMessage(MSG_PRIORITY_HIGH)));
	fContextMenu->AddItem(priorityMenu);

	// Calculate scaling
	BFont font;
	GetFont(&font);
	float scale = GetScaleFactor(&font);

	fPIDWidth = kBasePIDWidth * scale;
	fNameWidth = kBaseNameWidth * scale;
	fStateWidth = kBaseStateWidth * scale;
	fCPUWidth = kBaseCPUWidth * scale;
	fMemWidth = kBaseMemWidth * scale;
	fThreadsWidth = kBaseThreadsWidth * scale;
	fUserWidth = kBaseUserWidth * scale;

	// Header View construction
	BGroupView* headerView = new BGroupView(B_HORIZONTAL, 0);
	headerView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	// Helper to add header label
	auto addHeader = [&](const char* label, float width, int32 mode) {
		ClickableHeaderView* sv = new ClickableHeaderView(label, width, mode, this);
		headerView->AddChild(sv);
		fHeaders.push_back(sv);
	};

	addHeader(B_TRANSLATE("PID"), fPIDWidth, SORT_BY_PID);
	addHeader(B_TRANSLATE("Name"), fNameWidth, SORT_BY_NAME);
	addHeader(B_TRANSLATE("State"), fStateWidth, SORT_BY_STATE);
	addHeader(B_TRANSLATE("CPU%"), fCPUWidth, SORT_BY_CPU);
	addHeader(B_TRANSLATE("Mem"), fMemWidth, SORT_BY_MEM);
	addHeader(B_TRANSLATE("Thds"), fThreadsWidth, SORT_BY_THREADS);
	addHeader(B_TRANSLATE("User"), fUserWidth, SORT_BY_USER);

	headerView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 20 * scale));

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(0)
		.Add(fSearchControl)
		.Add(headerView)
		.Add(processScrollView)
	.End();
}

ProcessView::~ProcessView()
{
	fTerminated = true;
	if (fQuitSem >= 0)
		delete_sem(fQuitSem);
	if (fUpdateThread != B_ERROR) {
		status_t ret;
		wait_for_thread(fUpdateThread, &ret);
	}
	delete fContextMenu;

	fProcessListView->MakeEmpty(); // Just clears pointers
	fVisibleItems.clear();
	for (auto& pair : fTeamItemMap) {
		delete pair.second;
	}
	fTeamItemMap.clear();
}

void ProcessView::AttachedToWindow()
{
	BView::AttachedToWindow();
	fTerminated = false;
	fProcessListView->SetTarget(this);
	fSearchControl->SetTarget(this);
	fLastSystemTime = system_time();

	if (fQuitSem < 0)
		fQuitSem = create_sem(0, "ProcessView Quit");

	fThreadTimeMap.clear();
	fCachedTeamInfo.clear();

	fUpdateThread = spawn_thread(UpdateThread, "Process Update", B_NORMAL_PRIORITY, this);
	if (fUpdateThread >= 0)
		resume_thread(fUpdateThread);
}

void ProcessView::DetachedFromWindow()
{
	fTerminated = true;
	if (fQuitSem >= 0) {
		delete_sem(fQuitSem);
		fQuitSem = -1;
	}
	if (fUpdateThread != B_ERROR) {
		status_t ret;
		wait_for_thread(fUpdateThread, &ret);
		fUpdateThread = B_ERROR;
	}
}

void ProcessView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_KILL_PROCESS:
			KillSelectedProcess();
			break;
		case MSG_SUSPEND_PROCESS:
			SuspendSelectedProcess();
			break;
		case MSG_RESUME_PROCESS:
			ResumeSelectedProcess();
			break;
		case MSG_PRIORITY_LOW:
			SetSelectedProcessPriority(B_LOW_PRIORITY);
			break;
		case MSG_PRIORITY_NORMAL:
			SetSelectedProcessPriority(B_NORMAL_PRIORITY);
			break;
		case MSG_PRIORITY_HIGH:
			SetSelectedProcessPriority(B_URGENT_DISPLAY_PRIORITY);
			break;
		case MSG_PROCESS_DATA_UPDATE:
			Update(message);
			break;
		case MSG_SEARCH_UPDATED:
			FilterRows();
			break;
		case MSG_SHOW_CONTEXT_MENU: {
			BPoint screenWhere;
			if (message->FindPoint("screen_where", &screenWhere) == B_OK) {
				ShowContextMenu(screenWhere);
			}
			break;
		}
		case MSG_CONFIRM_KILL: {
			int32 button_index;
			team_id team;
			if (message->FindInt32("which", &button_index) == B_OK && button_index == 0) {
				if (message->FindInt32("team_id", (int32*)&team) == B_OK) {
					if (kill_team(team) != B_OK) {
						BAlert* errAlert = new BAlert(B_TRANSLATE("Error"), B_TRANSLATE("Failed to kill process."), B_TRANSLATE("OK"),
													  NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
						errAlert->Go(NULL);
					}
				}
			}
			break;
		}
		case MSG_HEADER_CLICKED: {
			int32 mode;
			if (message->FindInt32("mode", &mode) == B_OK) {
				fSortMode = (ProcessSortMode)mode;
				FilterRows(); // Trigger sort
			}
			break;
		}
		default:
			BView::MessageReceived(message);
			break;
	}
}

void ProcessView::KeyDown(const char* bytes, int32 numBytes)
{
	if (numBytes == 1) {
		if (bytes[0] == B_DELETE) {
			KillSelectedProcess();
			return;
		}
	}
	BView::KeyDown(bytes, numBytes);
}

void ProcessView::Hide()
{
	fIsHidden = true;
	BView::Hide();
}

void ProcessView::Show()
{
	fIsHidden = false;
	BView::Show();
}

BString ProcessView::GetUserName(uid_t uid, std::vector<char>& buffer) {
	auto it = fUserNameCache.find(uid);
	if (it != fUserNameCache.end()) {
		it->second.generation = fCurrentGeneration;
		return it->second.name;
	}

	struct passwd pwd;
	struct passwd* result = NULL;

	BString name;
	if (getpwuid_r(uid, &pwd, buffer.data(), buffer.size(), &result) == 0 && result != NULL) {
		name = result->pw_name;
	} else {
		name << uid;
	}

	fUserNameCache[uid] = CachedUser{name, fCurrentGeneration};
	return name;
}

void ProcessView::ShowContextMenu(BPoint screenPoint) {
	int32 selection = fProcessListView->CurrentSelection();
	if (selection < 0) return;

	fContextMenu->SetTargetForItems(this);
	fContextMenu->Go(screenPoint, true, true, true);
}

void ProcessView::KillSelectedProcess() {
	int32 selection = fProcessListView->CurrentSelection();
	if (selection < 0) return;

	ProcessListItem* item = dynamic_cast<ProcessListItem*>(fProcessListView->ItemAt(selection));
	if (!item) return;

	team_id team = item->TeamID();

	BString alertMsg;
	alertMsg.SetToFormat(B_TRANSLATE("Are you sure you want to kill process %d (%s)?"),
						 (int)team, item->Name());
	BAlert* confirmAlert = new BAlert(B_TRANSLATE("Confirm Kill"), alertMsg.String(), B_TRANSLATE("Kill"), B_TRANSLATE("Cancel"),
									  NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);

	BMessage* confirmMsg = new BMessage(MSG_CONFIRM_KILL);
	confirmMsg->AddInt32("team_id", team);
	confirmAlert->Go(new BInvoker(confirmMsg, this));
}

void ProcessView::SuspendSelectedProcess() {
	int32 selection = fProcessListView->CurrentSelection();
	if (selection < 0) return;
	ProcessListItem* item = dynamic_cast<ProcessListItem*>(fProcessListView->ItemAt(selection));
	if (!item) return;
	send_signal(item->TeamID(), SIGSTOP);
}

void ProcessView::ResumeSelectedProcess() {
	int32 selection = fProcessListView->CurrentSelection();
	if (selection < 0) return;
	ProcessListItem* item = dynamic_cast<ProcessListItem*>(fProcessListView->ItemAt(selection));
	if (!item) return;
	send_signal(item->TeamID(), SIGCONT);
}

void ProcessView::SetSelectedProcessPriority(int32 priority) {
	int32 selection = fProcessListView->CurrentSelection();
	if (selection < 0) return;
	ProcessListItem* item = dynamic_cast<ProcessListItem*>(fProcessListView->ItemAt(selection));
	if (!item) return;

	team_id team = item->TeamID();
	thread_info tInfo;
	int32 cookie = 0;
	while (get_next_thread_info(team, &cookie, &tInfo) == B_OK) {
		set_thread_priority(tInfo.thread, priority);
	}
}

void ProcessView::SetRefreshInterval(bigtime_t interval)
{
	fRefreshInterval = interval;
	if (fQuitSem >= 0)
		release_sem(fQuitSem);
}

void ProcessView::_SortItems()
{
	switch (fSortMode) {
		case SORT_BY_PID: fProcessListView->SortItems(ProcessListItem::ComparePID); break;
		case SORT_BY_NAME: fProcessListView->SortItems(ProcessListItem::CompareName); break;
		case SORT_BY_MEM: fProcessListView->SortItems(ProcessListItem::CompareMem); break;
		case SORT_BY_THREADS: fProcessListView->SortItems(ProcessListItem::CompareThreads); break;
		case SORT_BY_STATE: fProcessListView->SortItems(ProcessListItem::CompareState); break;
		case SORT_BY_USER: fProcessListView->SortItems(ProcessListItem::CompareUser); break;
		case SORT_BY_CPU: default: fProcessListView->SortItems(ProcessListItem::CompareCPU); break;
	}
}


void ProcessView::_RestoreSelection(team_id selectedID)
{
	if (selectedID == -1)
		return;

	for (int32 i = 0; i < fProcessListView->CountItems(); i++) {
		ProcessListItem* item = static_cast<ProcessListItem*>(fProcessListView->ItemAt(i));
		if (item && item->TeamID() == selectedID) {
			fProcessListView->Select(i);
			break;
		}
	}
}


void ProcessView::_UpdateHeaderWidths()
{
	float widths[] = { fPIDWidth, fNameWidth, fStateWidth, fCPUWidth, fMemWidth, fThreadsWidth, fUserWidth };
	size_t count = std::min(fHeaders.size(), sizeof(widths) / sizeof(widths[0]));
	for (size_t i = 0; i < count; i++) {
		fHeaders[i]->SetWidth(widths[i]);
	}
}


bool ProcessView::_MatchesFilter(const ProcessInfo& info, const char* searchText)
{
	if (searchText == NULL || strlen(searchText) == 0)
		return true;

	fFilterName.SetTo(info.name);
	fFilterID.SetToFormat("%" B_PRId32, info.id);
	fFilterArgs.SetTo(info.args);

	if (fFilterName.IFindFirst(searchText) != B_ERROR
		|| fFilterID.IFindFirst(searchText) != B_ERROR
		|| fFilterArgs.IFindFirst(searchText) != B_ERROR) {
		return true;
	}
	return false;
}

void ProcessView::FilterRows()
{
	const char* searchText = fSearchControl->Text();

	// Preserve selection
	int32 selection = fProcessListView->CurrentSelection();
	team_id selectedID = -1;
	if (selection >= 0) {
		ProcessListItem* item = dynamic_cast<ProcessListItem*>(fProcessListView->ItemAt(selection));
		if (item) selectedID = item->TeamID();
	}

	// BListView doesn't support hiding items easily.
	// We have to remove them from the list but keep them in fTeamItemMap.

	fProcessListView->MakeEmpty(); // Clear visualization (pointers only)
	fVisibleItems.clear();

	for (auto& pair : fTeamItemMap) {
		ProcessListItem* item = pair.second;

		if (_MatchesFilter(item->Info(), searchText)) {
			fProcessListView->AddItem(item);
			fVisibleItems.insert(item);
		}
	}

	_SortItems();

	_RestoreSelection(selectedID);

	fProcessListView->Invalidate();
}

void ProcessView::Update(BMessage* message)
{
	const void* data;
	ssize_t size;
	if (message->FindData("procs", B_RAW_TYPE, &data, &size) != B_OK)
		return;

	const ProcessInfo* infos = (const ProcessInfo*)data;
	size_t count = size / sizeof(ProcessInfo);

	const char* searchText = fSearchControl->Text();

	// Preserve selection
	int32 selection = fProcessListView->CurrentSelection();
	team_id selectedID = -1;
	if (selection >= 0) {
		ProcessListItem* item = dynamic_cast<ProcessListItem*>(fProcessListView->ItemAt(selection));
		if (item) selectedID = item->TeamID();
	}

	fListGeneration++;

	// Get font once
	BFont font;
	fProcessListView->GetFont(&font);

	bool fontChanged = (font != fCachedFont);
	if (fontChanged) {
		fCachedFont = font;
		float scale = GetScaleFactor(&font);
		fPIDWidth = kBasePIDWidth * scale;
		fNameWidth = kBaseNameWidth * scale;
		fStateWidth = kBaseStateWidth * scale;
		fCPUWidth = kBaseCPUWidth * scale;
		fMemWidth = kBaseMemWidth * scale;
		fThreadsWidth = kBaseThreadsWidth * scale;
		fUserWidth = kBaseUserWidth * scale;

		_UpdateHeaderWidths();
	}

	// First pass: Update existing items or create new ones
	for (size_t i = 0; i < count; i++) {
		const ProcessInfo& info = infos[i];

		const char* stateStr;
		switch (info.state) {
			case PROCESS_STATE_RUNNING:
				stateStr = fStrRunning.String();
				break;
			case PROCESS_STATE_READY:
				stateStr = fStrReady.String();
				break;
			case PROCESS_STATE_SLEEPING:
				stateStr = fStrSleeping.String();
				break;
			default:
				stateStr = "Unknown";
				break;
		}

		bool match = _MatchesFilter(info, searchText);

		ProcessListItem* item;
		auto result = fTeamItemMap.emplace(info.id, nullptr);
		if (result.second) {
			item = new ProcessListItem(info, stateStr, &font, this);
			result.first->second = item;

			if (match) {
				fProcessListView->AddItem(item);
				fVisibleItems.insert(item);
			}
		} else {
			item = result.first->second;
			bool wasVisible = fVisibleItems.find(item) != fVisibleItems.end();
			item->Update(info, stateStr, &font, fontChanged);

			if (match && !wasVisible) {
				fProcessListView->AddItem(item);
				fVisibleItems.insert(item);
			} else if (!match && wasVisible) {
				fProcessListView->RemoveItem(item);
				fVisibleItems.erase(item);
			}
		}
		item->SetGeneration(fListGeneration);
	}

	// Second pass: Remove dead processes
	for (auto it = fTeamItemMap.begin(); it != fTeamItemMap.end();) {
		if (it->second->Generation() != fListGeneration) {
			ProcessListItem* item = it->second;
			if (fVisibleItems.find(item) != fVisibleItems.end()) {
				fProcessListView->RemoveItem(item);
				fVisibleItems.erase(item);
			}
			delete item;
			it = fTeamItemMap.erase(it);
		} else {
			++it;
		}
	}

	_SortItems();

	_RestoreSelection(selectedID);

	fProcessListView->Invalidate();
}

int32 ProcessView::UpdateThread(void* data)
{
	// Implementation remains largely same, just logic to fetch data
	ProcessView* view = static_cast<ProcessView*>(data);
	BMessenger target(view);

	std::vector<ProcessInfo> procList;
	procList.reserve(128);

	long bufSize = sysconf(_SC_GETPW_R_SIZE_MAX);
	if (bufSize == -1) bufSize = 16384;
	std::vector<char> pwdBuffer(bufSize);

	while (!view->fTerminated) {
		if (view->fIsHidden) {
			status_t err = acquire_sem_etc(view->fQuitSem, 1, B_RELATIVE_TIMEOUT, view->fRefreshInterval);
			if (err != B_OK && err != B_TIMED_OUT && err != B_INTERRUPTED) break;
			continue;
		}

		view->fCurrentGeneration++;
		procList.clear();

		bigtime_t currentSystemTime = system_time();
		bigtime_t systemTimeDelta = currentSystemTime - view->fLastSystemTime;
		if (systemTimeDelta <= 0) systemTimeDelta = 1;
		view->fLastSystemTime = currentSystemTime;

		system_info sysInfo;
		get_system_info(&sysInfo);
		float totalPossibleCoreTime = sysInfo.cpu_count * systemTimeDelta;
		if (totalPossibleCoreTime <= 0) totalPossibleCoreTime = 1.0f;

		int32 cookie = 0;
		team_info teamInfo;
		while (get_next_team_info(&cookie, &teamInfo) == B_OK) {
			ProcessInfo currentProc;
			currentProc.id = teamInfo.team;
			currentProc.userID = teamInfo.uid;

			CachedTeamInfo* cachedInfo = nullptr;
			bool cached = false;
			bool memoryNeedsUpdate = true;
			auto it = view->fCachedTeamInfo.find(teamInfo.team);
			if (it != view->fCachedTeamInfo.end()) {
				if (teamInfo.uid == it->second.uid
					&& strncmp(teamInfo.args, it->second.args, 64) == 0) {
					cached = true;
					cachedInfo = &it->second;
					strlcpy(currentProc.name, it->second.name, B_OS_NAME_LENGTH);
					strlcpy(currentProc.userName, it->second.userName, B_OS_NAME_LENGTH);
					strlcpy(currentProc.args, it->second.args, sizeof(currentProc.args));
					it->second.generation = view->fCurrentGeneration;

					// Update user generation even if process is cached
					auto userIt = view->fUserNameCache.find(teamInfo.uid);
					if (userIt != view->fUserNameCache.end())
						userIt->second.generation = view->fCurrentGeneration;

					// Optimize memory calculation
					if (it->second.cachedAreaCount == teamInfo.area_count
						&& (view->fCurrentGeneration - it->second.memoryGeneration < kMemoryCacheGenerations)) {
						memoryNeedsUpdate = false;
						currentProc.memoryUsageBytes = it->second.memoryUsage;
					}
				}
			}

			if (!cached) {
				image_info imgInfo;
				int32 imgCookie = 0;
				if (get_next_image_info(teamInfo.team, &imgCookie, &imgInfo) == B_OK) {
					const char* leafName = strrchr(imgInfo.name, '/');
					if (leafName != NULL)
						strlcpy(currentProc.name, leafName + 1, B_OS_NAME_LENGTH);
					else
						strlcpy(currentProc.name, imgInfo.name, B_OS_NAME_LENGTH);
				} else {
					strlcpy(currentProc.name, teamInfo.args, B_OS_NAME_LENGTH);
					if (strlen(currentProc.name) == 0)
						strlcpy(currentProc.name, "system_daemon", B_OS_NAME_LENGTH);
				}

				BString userName = view->GetUserName(currentProc.userID, pwdBuffer);
				strlcpy(currentProc.userName, userName.String(), B_OS_NAME_LENGTH);

				strlcpy(currentProc.args, teamInfo.args, sizeof(currentProc.args));

				CachedTeamInfo info;
				strlcpy(info.name, currentProc.name, B_OS_NAME_LENGTH);
				strlcpy(info.userName, currentProc.userName, B_OS_NAME_LENGTH);
				strlcpy(info.args, teamInfo.args, 64);
				info.uid = teamInfo.uid;
				info.generation = view->fCurrentGeneration;
				// Initialization for new cache entry (memory updated later)
				info.memoryUsage = 0;
				info.cachedAreaCount = -1;
				info.memoryGeneration = 0;
				info.cpuTime = 0;
				info.lastRunningThread = -1;
				view->fCachedTeamInfo[teamInfo.team] = info;
				cachedInfo = &view->fCachedTeamInfo[teamInfo.team];
			}

			currentProc.threadCount = teamInfo.thread_count;
			currentProc.areaCount = teamInfo.area_count;

			int32 threadCookie = 0;
			thread_info tInfo;
			bigtime_t teamActiveTimeDelta = 0;

			bool isRunning = false;
			bool isReady = false;

			if (teamInfo.team == 1) { // Kernel team: use thread iteration
				while (get_next_thread_info(teamInfo.team, &threadCookie, &tInfo) == B_OK) {
					bigtime_t threadTime = tInfo.user_time + tInfo.kernel_time;

					if (tInfo.state == B_THREAD_RUNNING) isRunning = true;
					if (tInfo.state == B_THREAD_READY) isReady = true;

					auto it = view->fThreadTimeMap.find(tInfo.thread);
					if (it != view->fThreadTimeMap.end()) {
						bigtime_t threadTimeDelta = threadTime - it->second.time;
						if (threadTimeDelta < 0) threadTimeDelta = 0;

						if (strstr(tInfo.name, "idle thread") == NULL) {
							teamActiveTimeDelta += threadTimeDelta;
						}
						it->second.time = threadTime;
						it->second.generation = view->fCurrentGeneration;
					} else {
						view->fThreadTimeMap.emplace(tInfo.thread, ThreadState{threadTime, view->fCurrentGeneration});
					}
				}
			} else { // Regular teams: use bulk API and optimized state check
				team_usage_info usageInfo;
				bool skipThreadScan = false;
				if (get_team_usage_info(teamInfo.team, B_TEAM_USAGE_SELF, &usageInfo) == B_OK) {
					bigtime_t currentTeamTime = usageInfo.user_time + usageInfo.kernel_time;
					if (cached) {
						teamActiveTimeDelta = currentTeamTime - cachedInfo->cpuTime;
						if (teamActiveTimeDelta < 0) teamActiveTimeDelta = 0;
					}
					cachedInfo->cpuTime = currentTeamTime;
				}

				if (!skipThreadScan) {
					// Optimization: Check the last known running thread first
					if (cached && cachedInfo->lastRunningThread != -1) {
						thread_info lastInfo;
						if (get_thread_info(cachedInfo->lastRunningThread, &lastInfo) == B_OK
							&& lastInfo.team == teamInfo.team
							&& lastInfo.state == B_THREAD_RUNNING) {
							isRunning = true;
							skipThreadScan = true;
						}
					}

					if (!skipThreadScan) {
						while (get_next_thread_info(teamInfo.team, &threadCookie, &tInfo) == B_OK) {
							if (tInfo.state == B_THREAD_RUNNING) {
								isRunning = true;
								cachedInfo->lastRunningThread = tInfo.thread;
								break; // Found running, can stop scanning
							}
							if (tInfo.state == B_THREAD_READY) isReady = true;
						}
					}
				}
			}

			if (isRunning) currentProc.state = PROCESS_STATE_RUNNING;
			else if (isReady) currentProc.state = PROCESS_STATE_READY;
			else currentProc.state = PROCESS_STATE_SLEEPING;

			float teamCpuPercent = (float)teamActiveTimeDelta / totalPossibleCoreTime * 100.0f;
			if (teamCpuPercent < 0.0f) teamCpuPercent = 0.0f;
			if (teamCpuPercent > 100.0f) teamCpuPercent = 100.0f;
			currentProc.cpuUsage = teamCpuPercent;

			if (memoryNeedsUpdate) {
				currentProc.memoryUsageBytes = 0;
				area_info areaInfo;
				ssize_t areaCookie = 0;
				while (get_next_area_info(teamInfo.team, &areaCookie, &areaInfo) == B_OK) {
					currentProc.memoryUsageBytes += areaInfo.ram_size;
				}

				// Update cache
				auto it = view->fCachedTeamInfo.find(teamInfo.team);
				if (it != view->fCachedTeamInfo.end()) {
					it->second.memoryUsage = currentProc.memoryUsageBytes;
					it->second.cachedAreaCount = teamInfo.area_count;
					it->second.memoryGeneration = view->fCurrentGeneration;
				}
			}

			procList.push_back(currentProc);
		}

		for (auto it = view->fThreadTimeMap.begin(); it != view->fThreadTimeMap.end();) {
			if (it->second.generation != view->fCurrentGeneration)
				it = view->fThreadTimeMap.erase(it);
			else
				++it;
		}

		for (auto it = view->fCachedTeamInfo.begin(); it != view->fCachedTeamInfo.end();) {
			if (it->second.generation != view->fCurrentGeneration)
				it = view->fCachedTeamInfo.erase(it);
			else
				++it;
		}

		for (auto it = view->fUserNameCache.begin(); it != view->fUserNameCache.end();) {
			if (it->second.generation != view->fCurrentGeneration)
				it = view->fUserNameCache.erase(it);
			else
				++it;
		}

		if (!procList.empty()) {
			BMessage msg(MSG_PROCESS_DATA_UPDATE);
			msg.AddData("procs", B_RAW_TYPE, procList.data(), procList.size() * sizeof(ProcessInfo));
			target.SendMessage(&msg);
		}

		status_t err = acquire_sem_etc(view->fQuitSem, 1, B_RELATIVE_TIMEOUT, view->fRefreshInterval);
		if (err == B_OK) {
			int32 count;
			if (get_sem_count(view->fQuitSem, &count) == B_OK && count > 0)
				acquire_sem_etc(view->fQuitSem, count, B_RELATIVE_TIMEOUT, 0);
		} else if (err != B_TIMED_OUT && err != B_INTERRUPTED) {
			break;
		}
	}

	return B_OK;
}

void ProcessView::SaveState(BMessage& state)
{
	// Not implemented for BListView version yet
}

void ProcessView::LoadState(const BMessage& state)
{
	// Not implemented for BListView version yet
}
