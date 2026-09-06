#ifndef PROCESSLISTITEM_H
#define PROCESSLISTITEM_H

#include <ListItem.h>
#include <String.h>
#include <Font.h>
#include <View.h>
#include <InterfaceDefs.h>
#include <cstring>
#include <Catalog.h>
#include "ProcessView.h"
#include "Utils.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ProcessListItem"

class ProcessListItem : public BListItem {
public:
	using BListItem::Update;
	ProcessListItem(const ProcessInfo& info, const char* stateStr,
		const BFont* font, ProcessView* view)
		: BListItem(), fGeneration(0), fView(view), fIsVisible(false)
	{
		Update(info, stateStr, font, true);
	}

	void SetGeneration(int32 generation) { fGeneration = generation; }
	int32 Generation() const { return fGeneration; }

	bool IsVisible() const { return fIsVisible; }
	void SetVisible(bool visible) { fIsVisible = visible; }

	void Update(const ProcessInfo& info, const char* stateStr,
		const BFont* font, bool force = false)
	{
		bool nameChanged    = force || strcmp(fInfo.name, info.name) != 0;
		bool userChanged    = force || strcmp(fInfo.userName, info.userName) != 0;
		bool stateChanged   = force || fInfo.state != info.state;
		bool cpuChanged     = force || fInfo.cpuUsage != info.cpuUsage;
		bool memChanged     = force || fInfo.memoryUsageBytes != info.memoryUsageBytes;
		bool threadsChanged = force || fInfo.threadCount != info.threadCount;
		bool priorityChanged = force || fInfo.priority != info.priority;
		bool pidChanged     = force || fInfo.id != info.id;

		fInfo = info;

		if (pidChanged)
			fCachedPID.SetToFormat("%" B_PRId32, fInfo.id);

		if (nameChanged) {
			if (font && fView) {
				fTruncatedName = fInfo.name;
				font->TruncateString(&fTruncatedName, B_TRUNCATE_END,
					fView->NameWidth() - 10);
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

		if (priorityChanged) {
			if (fInfo.priority <= B_LOW_PRIORITY) fCachedPriority = B_TRANSLATE("Low");
			else if (fInfo.priority <= B_NORMAL_PRIORITY) fCachedPriority = B_TRANSLATE("Normal");
			else if (fInfo.priority <= B_DISPLAY_PRIORITY) fCachedPriority = B_TRANSLATE("High");
			else if (fInfo.priority <= B_REAL_TIME_DISPLAY_PRIORITY) fCachedPriority = B_TRANSLATE("Real-Time");
			else fCachedPriority.SetToFormat("%" B_PRId32, fInfo.priority);
		}

		if (userChanged) {
			if (font && fView) {
				fTruncatedUser = fInfo.userName;
				font->TruncateString(&fTruncatedUser, B_TRUNCATE_END,
					fView->UserWidth() - 10);
			} else {
				fTruncatedUser = fInfo.userName;
			}
		}
	}

	virtual void DrawItem(BView* owner, BRect itemRect, bool complete = false) {
		if (!fView) return;
		if (IsSelected() || complete) {
			rgb_color color = IsSelected()
				? ui_color(B_LIST_SELECTED_BACKGROUND_COLOR)
				: ui_color(B_LIST_BACKGROUND_COLOR);
			owner->SetHighColor(color);
			owner->FillRect(itemRect);
		}

		rgb_color textColor = IsSelected()
			? ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR)
			: ui_color(B_LIST_ITEM_TEXT_COLOR);
		owner->SetHighColor(textColor);

		font_height fh;
		owner->GetFontHeight(&fh);
		float x = itemRect.left + 5;
		float y = itemRect.bottom - fh.descent;

		float rightX = 0;
		rightX = x + fView->PIDWidth() - owner->StringWidth(fCachedPID.String()) - 5;
		owner->DrawString(fCachedPID.String(), BPoint(rightX, y));
		x += fView->PIDWidth();

		owner->DrawString(fTruncatedName.String(), BPoint(x, y)); x += fView->NameWidth();
		owner->DrawString(fCachedState.String(),   BPoint(x, y)); x += fView->StateWidth();

		rightX = x + fView->CPUWidth() - owner->StringWidth(fCachedCPU.String()) - 5;
		owner->DrawString(fCachedCPU.String(),     BPoint(rightX, y));
		x += fView->CPUWidth();

		rightX = x + fView->MemWidth() - owner->StringWidth(fCachedMem.String()) - 5;
		owner->DrawString(fCachedMem.String(),     BPoint(rightX, y));
		x += fView->MemWidth();

		rightX = x + fView->ThreadsWidth() - owner->StringWidth(fCachedThreads.String()) - 5;
		owner->DrawString(fCachedThreads.String(), BPoint(rightX, y));
		x += fView->ThreadsWidth();

		owner->DrawString(fCachedPriority.String(), BPoint(x, y)); x += fView->PriorityWidth();
		owner->DrawString(fTruncatedUser.String(), BPoint(x, y));
	}

	team_id             TeamID() const { return fInfo.id; }
	const char*         Name()   const { return fInfo.name; }
	const ProcessInfo&  Info()   const { return fInfo; }

	static bool sSortAscending;

	static int CompareCPU(const void* a, const void* b) {
		const ProcessListItem* i1 = *static_cast<const ProcessListItem* const*>(a);
		const ProcessListItem* i2 = *static_cast<const ProcessListItem* const*>(b);
		int result = 0;
		if (i1->fInfo.cpuUsage > i2->fInfo.cpuUsage) result = -1;
		else if (i1->fInfo.cpuUsage < i2->fInfo.cpuUsage) result = 1;
		return sSortAscending ? -result : result;
	}
	static int ComparePID(const void* a, const void* b) {
		const ProcessListItem* i1 = *static_cast<const ProcessListItem* const*>(a);
		const ProcessListItem* i2 = *static_cast<const ProcessListItem* const*>(b);
		int result = 0;
		if (i1->fInfo.id < i2->fInfo.id) result = -1;
		else if (i1->fInfo.id > i2->fInfo.id) result = 1;
		return sSortAscending ? result : -result;
	}
	static int CompareName(const void* a, const void* b) {
		const ProcessListItem* i1 = *static_cast<const ProcessListItem* const*>(a);
		const ProcessListItem* i2 = *static_cast<const ProcessListItem* const*>(b);
		int result = strcasecmp(i1->fInfo.name, i2->fInfo.name);
		return sSortAscending ? result : -result;
	}
	static int CompareMem(const void* a, const void* b) {
		const ProcessListItem* i1 = *static_cast<const ProcessListItem* const*>(a);
		const ProcessListItem* i2 = *static_cast<const ProcessListItem* const*>(b);
		int result = 0;
		if (i1->fInfo.memoryUsageBytes > i2->fInfo.memoryUsageBytes) result = -1;
		else if (i1->fInfo.memoryUsageBytes < i2->fInfo.memoryUsageBytes) result = 1;
		return sSortAscending ? -result : result;
	}
	static int CompareThreads(const void* a, const void* b) {
		const ProcessListItem* i1 = *static_cast<const ProcessListItem* const*>(a);
		const ProcessListItem* i2 = *static_cast<const ProcessListItem* const*>(b);
		int result = 0;
		if (i1->fInfo.threadCount > i2->fInfo.threadCount) result = -1;
		else if (i1->fInfo.threadCount < i2->fInfo.threadCount) result = 1;
		return sSortAscending ? -result : result;
	}
	static int ComparePriority(const void* a, const void* b) {
		const ProcessListItem* i1 = *static_cast<const ProcessListItem* const*>(a);
		const ProcessListItem* i2 = *static_cast<const ProcessListItem* const*>(b);
		int result = 0;
		if (i1->fInfo.priority > i2->fInfo.priority) result = -1;
		else if (i1->fInfo.priority < i2->fInfo.priority) result = 1;
		return sSortAscending ? -result : result;
	}
	static int CompareState(const void* a, const void* b) {
		const ProcessListItem* i1 = *static_cast<const ProcessListItem* const*>(a);
		const ProcessListItem* i2 = *static_cast<const ProcessListItem* const*>(b);
		int result = 0;
		if (i1->fInfo.state < i2->fInfo.state) result = -1;
		else if (i1->fInfo.state > i2->fInfo.state) result = 1;
		return sSortAscending ? result : -result;
	}
	static int CompareUser(const void* a, const void* b) {
		const ProcessListItem* i1 = *static_cast<const ProcessListItem* const*>(a);
		const ProcessListItem* i2 = *static_cast<const ProcessListItem* const*>(b);
		int result = strcasecmp(i1->fInfo.userName, i2->fInfo.userName);
		return sSortAscending ? result : -result;
	}

private:
	ProcessInfo	fInfo;
	BString		fCachedPID;
	BString		fCachedState;
	BString		fCachedCPU;
	BString		fCachedMem;
	BString		fCachedThreads;
	BString		fCachedPriority;
	BString		fTruncatedName;
	BString		fTruncatedUser;
	int32		fGeneration;
	ProcessView* fView;
	bool		fIsVisible;
};

#endif // PROCESSLISTITEM_H
