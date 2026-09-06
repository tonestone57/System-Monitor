#ifndef DISKLISTITEM_H
#define DISKLISTITEM_H

#include <ListItem.h>
#include <String.h>
#include <Font.h>
#include <View.h>
#include <InterfaceDefs.h>
#include <SupportDefs.h>
#include <cstring>
#include <Volume.h>
#include <Bitmap.h>
#include <Catalog.h>
#include "DiskView.h"
#include "Utils.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DiskListItem"

class DiskListItem : public BListItem {
public:
	using BListItem::Update;
	DiskListItem(dev_t deviceID,
		const BString& device, const BString& mount, const BString& fs,
		uint64 total, uint64 used, uint64 free, double percent,
		const BFont* font, DiskView* view)
		: BListItem(), fGeneration(0), fDeviceID(deviceID), fView(view), fIcon(NULL)
	{
		_UpdateIcon();
		Update(device, mount, fs, total, used, free, percent, font, true);
	}

	virtual ~DiskListItem() {
		delete fIcon;
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
					fView->DeviceWidth() - 25);
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

		// Draw volume icon or placeholder
		BRect iconRect(x, itemRect.top + (itemRect.Height() - 16) / 2, x + 15, itemRect.top + (itemRect.Height() - 16) / 2 + 15);
		if (fIcon != NULL) {
			owner->SetDrawingMode(B_OP_OVER);
			owner->DrawBitmap(fIcon, iconRect.LeftTop());
			owner->SetDrawingMode(B_OP_COPY);
		} else {
			owner->SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
			owner->FillRect(iconRect);
		}
		owner->SetHighColor(textColor); // Restore text color

		float deviceStringX = x + 20; // Icon width (16) + padding (4)
		owner->DrawString(fTruncatedDevice.String(), BPoint(deviceStringX, y));
		x += fView->DeviceWidth();
		owner->DrawString(fTruncatedMount.String(),  BPoint(x, y)); x += fView->MountWidth();
		owner->DrawString(fTruncatedFS.String(),     BPoint(x, y)); x += fView->FSWidth();
		drawRight(fCachedTotal,   fView->TotalWidth());
		drawRight(fCachedUsed,    fView->UsedWidth());
		drawRight(fCachedFree,    fView->FreeWidth());

		float barX = x + 10;
		float barWidth = itemRect.right - barX - 10;
		if (barWidth > 20) {
			BRect barRect(barX, itemRect.top + 2, barX + barWidth, itemRect.bottom - 2);

			// Background
			rgb_color darkBg = ui_color(B_PANEL_BACKGROUND_COLOR);
			owner->SetHighColor(darkBg);
			owner->FillRect(barRect);

			// Fill
			if (fPercent > 0) {
				BRect fillRect = barRect;
				fillRect.right = fillRect.left + (barWidth * (fPercent / 100.0));

				rgb_color customColor = make_color(255, 207, 0, 255);
				owner->SetHighColor(customColor);
				owner->FillRect(fillRect);
			}

			// Percentage text in the center of the bar
			// Set drawing mode to ensure text is visible over background
			owner->SetDrawingMode(B_OP_OVER);
			rgb_color blackColor = make_color(0, 0, 0, 255);
			owner->SetHighColor(blackColor);
			BString percentStr;
			percentStr.SetToFormat("%.0f%%", fPercent);
			float percentWidth = owner->StringWidth(percentStr.String());
			float percentX = barRect.left + (barWidth - percentWidth) / 2.0;
			owner->DrawString(percentStr.String(), BPoint(percentX, y));

			// Restore drawing mode
			owner->SetDrawingMode(B_OP_COPY);
		}
		x += fView->PercentWidth();
	}

	static bool sSortAscending;

	static int CompareDevice(const void* a, const void* b) {
		const DiskListItem* i1 = *static_cast<const DiskListItem* const*>(a);
		const DiskListItem* i2 = *static_cast<const DiskListItem* const*>(b);
		int result = strcasecmp(i1->fDevice.String(), i2->fDevice.String());
		return sSortAscending ? result : -result;
	}
	static int CompareMount(const void* a, const void* b) {
		const DiskListItem* i1 = *static_cast<const DiskListItem* const*>(a);
		const DiskListItem* i2 = *static_cast<const DiskListItem* const*>(b);
		int result = strcasecmp(i1->fMount.String(), i2->fMount.String());
		return sSortAscending ? result : -result;
	}
	static int CompareFS(const void* a, const void* b) {
		const DiskListItem* i1 = *static_cast<const DiskListItem* const*>(a);
		const DiskListItem* i2 = *static_cast<const DiskListItem* const*>(b);
		int result = strcasecmp(i1->fFS.String(), i2->fFS.String());
		return sSortAscending ? result : -result;
	}
	static int CompareTotal(const void* a, const void* b) {
		const DiskListItem* i1 = *static_cast<const DiskListItem* const*>(a);
		const DiskListItem* i2 = *static_cast<const DiskListItem* const*>(b);
		int result = 0;
		if (i1->fTotal > i2->fTotal) result = -1;
		else if (i1->fTotal < i2->fTotal) result = 1;
		return sSortAscending ? -result : result;
	}
	static int CompareUsed(const void* a, const void* b) {
		const DiskListItem* i1 = *static_cast<const DiskListItem* const*>(a);
		const DiskListItem* i2 = *static_cast<const DiskListItem* const*>(b);
		int result = 0;
		if (i1->fUsed > i2->fUsed) result = -1;
		else if (i1->fUsed < i2->fUsed) result = 1;
		return sSortAscending ? -result : result;
	}
	static int CompareFree(const void* a, const void* b) {
		const DiskListItem* i1 = *static_cast<const DiskListItem* const*>(a);
		const DiskListItem* i2 = *static_cast<const DiskListItem* const*>(b);
		int result = 0;
		if (i1->fFree > i2->fFree) result = -1;
		else if (i1->fFree < i2->fFree) result = 1;
		return sSortAscending ? -result : result;
	}
	static int CompareUsage(const void* a, const void* b) {
		const DiskListItem* i1 = *static_cast<const DiskListItem* const*>(a);
		const DiskListItem* i2 = *static_cast<const DiskListItem* const*>(b);
		int result = 0;
		if (i1->fPercent > i2->fPercent) result = -1;
		else if (i1->fPercent < i2->fPercent) result = 1;
		return sSortAscending ? -result : result;
	}

private:
	void _UpdateIcon() {
		if (fIcon != NULL) return;
		BVolume volume(fDeviceID);
		if (volume.InitCheck() == B_OK) {
			BBitmap* icon = new(std::nothrow) BBitmap(BRect(0, 0, 15, 15), B_RGBA32);
			if (icon != NULL && icon->InitCheck() == B_OK && volume.GetIcon(icon, B_MINI_ICON) == B_OK) {
				fIcon = icon;
			} else {
				delete icon;
			}
		}
	}

	BString  fDevice, fMount, fFS;
	uint64   fTotal, fUsed, fFree;
	double   fPercent;
	BString  fCachedTotal, fCachedUsed, fCachedFree, fCachedPercent;
	BString  fTruncatedDevice, fTruncatedMount, fTruncatedFS;
	int32    fGeneration;
	dev_t    fDeviceID;
	DiskView* fView;
	BBitmap* fIcon;
};

#endif // DISKLISTITEM_H
