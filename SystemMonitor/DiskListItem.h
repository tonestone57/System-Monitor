#ifndef DISKLISTITEM_H
#define DISKLISTITEM_H

#include <ListItem.h>
#include <String.h>
#include <Font.h>
#include <View.h>
#include <InterfaceDefs.h>
#include <SupportDefs.h>
#include <cstring>
#include "DiskView.h"
#include "Utils.h"

class DiskListItem : public BListItem {
public:
	using BListItem::Update;
	DiskListItem(dev_t deviceID,
		const BString& device, const BString& mount, const BString& fs,
		uint64 total, uint64 used, uint64 free, double percent,
		const BFont* font, DiskView* view)
		: BListItem(), fGeneration(0), fDeviceID(deviceID), fView(view)
	{
		Update(device, mount, fs, total, used, free, percent, font, true);
	}

	void SetGeneration(int32 generation) { fGeneration = generation; }
	int32 Generation() const { return fGeneration; }
	dev_t DeviceID()   const { return fDeviceID; }

	void Update(const BString& device, const BString& mount, const BString& fs,
		uint64 total, uint64 used, uint64 free, double percent,
		const BFont* font, bool force = false)
	{
		bool deviceChanged  = force || fDevice  != device;
		bool mountChanged   = force || fMount   != mount;
		bool fsChanged      = force || fFS      != fs;
		bool totalChanged   = force || fTotal   != total;
		bool usedChanged    = force || fUsed    != used;
		bool freeChanged    = force || fFree    != free;
		bool percentChanged = force || fPercent != percent;

		fDevice = device; fMount = mount; fFS = fs;
		fTotal = total; fUsed = used; fFree = free; fPercent = percent;

		if (totalChanged)   FormatBytes(fCachedTotal,   fTotal);
		if (usedChanged)    FormatBytes(fCachedUsed,    fUsed);
		if (freeChanged)    FormatBytes(fCachedFree,    fFree);
		if (percentChanged) fCachedPercent.SetToFormat("%.1f%%", fPercent);

		if (deviceChanged) {
			fTruncatedDevice = fDevice;
			if (font && fView)
				font->TruncateString(&fTruncatedDevice, B_TRUNCATE_MIDDLE,
					fView->DeviceWidth() - 10);
		}
		if (mountChanged) {
			fTruncatedMount = fMount;
			if (font && fView)
				font->TruncateString(&fTruncatedMount, B_TRUNCATE_MIDDLE,
					fView->MountWidth() - 10);
		}
		if (fsChanged) {
			fTruncatedFS = fFS;
			if (font && fView)
				font->TruncateString(&fTruncatedFS, B_TRUNCATE_END,
					fView->FSWidth() - 10);
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

		auto drawRight = [&](const BString& str, float width) {
			float w = owner->StringWidth(str.String());
			owner->DrawString(str.String(), BPoint(x + width - w - 5, y));
			x += width;
		};

		owner->DrawString(fTruncatedDevice.String(), BPoint(x, y)); x += fView->DeviceWidth();
		owner->DrawString(fTruncatedMount.String(),  BPoint(x, y)); x += fView->MountWidth();
		owner->DrawString(fTruncatedFS.String(),     BPoint(x, y)); x += fView->FSWidth();
		drawRight(fCachedTotal,   fView->TotalWidth());
		drawRight(fCachedFree,    fView->FreeWidth());

		// Draw "Used" text left-aligned, then progress bar
		owner->DrawString(fCachedUsed.String(), BPoint(x, y));
		float usedTextWidth = owner->StringWidth(fCachedUsed.String());

		float barX = x + usedTextWidth + 10;
		float barWidth = fView->UsedWidth() - usedTextWidth - 15;
		if (barWidth > 20) {
			BRect barRect(barX, itemRect.top + 2, barX + barWidth, itemRect.bottom - 2);

			// Background
			rgb_color bg = ui_color(B_PANEL_BACKGROUND_COLOR);
			rgb_color darkBg = { (uint8)(bg.red * 0.8), (uint8)(bg.green * 0.8), (uint8)(bg.blue * 0.8), 255 };
			owner->SetHighColor(darkBg);
			owner->FillRect(barRect);

			// Fill
			if (fPercent > 0) {
				BRect fillRect = barRect;
				fillRect.right = fillRect.left + (barWidth * (fPercent / 100.0));
				rgb_color blueColor = {40, 115, 235, 255};
				owner->SetHighColor(blueColor); // Blue color like in the screenshot
				owner->FillRect(fillRect);
			}

			// Percentage text in the center of the bar
			owner->SetHighColor(ui_color(B_CONTROL_TEXT_COLOR));
			BString percentStr;
			percentStr.SetToFormat("%.0f%%", fPercent);
			float percentWidth = owner->StringWidth(percentStr.String());
			float percentX = barRect.left + (barWidth - percentWidth) / 2.0;
			owner->DrawString(percentStr.String(), BPoint(percentX, y));
		}
		x += fView->UsedWidth();
	}

	static int CompareDevice(const void* a, const void* b) {
		const DiskListItem* i1 = *static_cast<const DiskListItem* const*>(a);
		const DiskListItem* i2 = *static_cast<const DiskListItem* const*>(b);
		return strcasecmp(i1->fDevice.String(), i2->fDevice.String());
	}
	static int CompareMount(const void* a, const void* b) {
		const DiskListItem* i1 = *static_cast<const DiskListItem* const*>(a);
		const DiskListItem* i2 = *static_cast<const DiskListItem* const*>(b);
		return strcasecmp(i1->fMount.String(), i2->fMount.String());
	}
	static int CompareFS(const void* a, const void* b) {
		const DiskListItem* i1 = *static_cast<const DiskListItem* const*>(a);
		const DiskListItem* i2 = *static_cast<const DiskListItem* const*>(b);
		return strcasecmp(i1->fFS.String(), i2->fFS.String());
	}
	static int CompareTotal(const void* a, const void* b) {
		const DiskListItem* i1 = *static_cast<const DiskListItem* const*>(a);
		const DiskListItem* i2 = *static_cast<const DiskListItem* const*>(b);
		if (i1->fTotal > i2->fTotal) return -1;
		if (i1->fTotal < i2->fTotal) return  1;
		return 0;
	}
	static int CompareUsed(const void* a, const void* b) {
		const DiskListItem* i1 = *static_cast<const DiskListItem* const*>(a);
		const DiskListItem* i2 = *static_cast<const DiskListItem* const*>(b);
		if (i1->fUsed > i2->fUsed) return -1;
		if (i1->fUsed < i2->fUsed) return  1;
		return 0;
	}
	static int CompareFree(const void* a, const void* b) {
		const DiskListItem* i1 = *static_cast<const DiskListItem* const*>(a);
		const DiskListItem* i2 = *static_cast<const DiskListItem* const*>(b);
		if (i1->fFree > i2->fFree) return -1;
		if (i1->fFree < i2->fFree) return  1;
		return 0;
	}
	static int CompareUsage(const void* a, const void* b) {
		const DiskListItem* i1 = *static_cast<const DiskListItem* const*>(a);
		const DiskListItem* i2 = *static_cast<const DiskListItem* const*>(b);
		if (i1->fPercent > i2->fPercent) return -1;
		if (i1->fPercent < i2->fPercent) return  1;
		return 0;
	}

private:
	BString  fDevice, fMount, fFS;
	uint64   fTotal, fUsed, fFree;
	double   fPercent;
	BString  fCachedTotal, fCachedUsed, fCachedFree, fCachedPercent;
	BString  fTruncatedDevice, fTruncatedMount, fTruncatedFS;
	int32    fGeneration;
	dev_t    fDeviceID;
	DiskView* fView;
};

#endif // DISKLISTITEM_H
