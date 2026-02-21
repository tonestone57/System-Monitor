#ifndef PROCESSLISTITEM_H
#define PROCESSLISTITEM_H

#include <ListItem.h>
#include <String.h>
#include <Font.h>
#include <View.h>
#include <InterfaceDefs.h>
#include <cstring>
#include "ProcessView.h"
#include "Utils.h"

class ProcessListItem : public BListItem {
public:
	ProcessListItem(const ProcessInfo& info, const char* stateStr,
		const BFont* font, ProcessView* view)
		: BListItem(), fGeneration(0), fView(view)
	{
		Update(info, stateStr, font, true);
	}

	void SetGeneration(int32 generation) { fGeneration = generation; }
	int32 Generation() const { return fGeneration; }

	void Update(const ProcessInfo& info, const char* stateStr,
		const BFont* font, bool force = false)
	{
		bool nameChanged    = force || strcmp(fInfo.name, info.name) != 0;
		bool userChanged    = force || strcmp(fInfo.userName, info.userName) != 0;
		bool stateChanged   = force || fInfo.state != info.state;
		bool cpuChanged     = force || fInfo.cpuUsage != info.cpuUsage;
		bool memChanged     = force || fInfo.memoryUsageBytes != info.memoryUsageBytes;
		bool threadsChanged = force || fInfo.threadCount != info.threadCount;
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

		owner->DrawString(fCachedPID.String(),    BPoint(x, y)); x += fView->PIDWidth();
		owner->DrawString(fTruncatedName.String(), BPoint(x, y)); x += fView->NameWidth();
		owner->DrawString(fCachedState.String(),   BPoint(x, y)); x += fView->StateWidth();
		owner->DrawString(fCachedCPU.String(),     BPoint(x, y)); x += fView->CPUWidth();
		owner->DrawString(fCachedMem.String(),     BPoint(x, y)); x += fView->MemWidth();
		owner->DrawString(fCachedThreads.String(), BPoint(x, y)); x += fView->ThreadsWidth();
		owner->DrawString(fTruncatedUser.String(), BPoint(x, y));
	}

	team_id             TeamID() const { return fInfo.id; }
	const char*         Name()   const { return fInfo.name; }
	const ProcessInfo&  Info()   const { return fInfo; }

	static int CompareCPU(const void* a, const void* b) {
		const ProcessListItem* i1 = *static_cast<const ProcessListItem* const*>(a);
		const ProcessListItem* i2 = *static_cast<const ProcessListItem* const*>(b);
		if (i1->fInfo.cpuUsage > i2->fInfo.cpuUsage) return -1;
		if (i1->fInfo.cpuUsage < i2->fInfo.cpuUsage) return  1;
		return 0;
	}
	static int ComparePID(const void* a, const void* b) {
		const ProcessListItem* i1 = *static_cast<const ProcessListItem* const*>(a);
		const ProcessListItem* i2 = *static_cast<const ProcessListItem* const*>(b);
		if (i1->fInfo.id < i2->fInfo.id) return -1;
		if (i1->fInfo.id > i2->fInfo.id) return  1;
		return 0;
	}
	static int CompareName(const void* a, const void* b) {
		const ProcessListItem* i1 = *static_cast<const ProcessListItem* const*>(a);
		const ProcessListItem* i2 = *static_cast<const ProcessListItem* const*>(b);
		return strcasecmp(i1->fInfo.name, i2->fInfo.name);
	}
	static int CompareMem(const void* a, const void* b) {
		const ProcessListItem* i1 = *static_cast<const ProcessListItem* const*>(a);
		const ProcessListItem* i2 = *static_cast<const ProcessListItem* const*>(b);
		if (i1->fInfo.memoryUsageBytes > i2->fInfo.memoryUsageBytes) return -1;
		if (i1->fInfo.memoryUsageBytes < i2->fInfo.memoryUsageBytes) return  1;
		return 0;
	}
	static int CompareThreads(const void* a, const void* b) {
		const ProcessListItem* i1 = *static_cast<const ProcessListItem* const*>(a);
		const ProcessListItem* i2 = *static_cast<const ProcessListItem* const*>(b);
		if (i1->fInfo.threadCount > i2->fInfo.threadCount) return -1;
		if (i1->fInfo.threadCount < i2->fInfo.threadCount) return  1;
		return 0;
	}
	static int CompareState(const void* a, const void* b) {
		const ProcessListItem* i1 = *static_cast<const ProcessListItem* const*>(a);
		const ProcessListItem* i2 = *static_cast<const ProcessListItem* const*>(b);
		if (i1->fInfo.state < i2->fInfo.state) return -1;
		if (i1->fInfo.state > i2->fInfo.state) return  1;
		return 0;
	}
	static int CompareUser(const void* a, const void* b) {
		const ProcessListItem* i1 = *static_cast<const ProcessListItem* const*>(a);
		const ProcessListItem* i2 = *static_cast<const ProcessListItem* const*>(b);
		return strcasecmp(i1->fInfo.userName, i2->fInfo.userName);
	}

private:
	ProcessInfo	fInfo;
	BString		fCachedPID;
	BString		fCachedState;
	BString		fCachedCPU;
	BString		fCachedMem;
	BString		fCachedThreads;
	BString		fTruncatedName;
	BString		fTruncatedUser;
	int32		fGeneration;
	ProcessView* fView;
};

#endif // PROCESSLISTITEM_H
