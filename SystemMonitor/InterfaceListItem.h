#ifndef INTERFACELISTITEM_H
#define INTERFACELISTITEM_H

#include <ListItem.h>
#include <String.h>
#include <Font.h>
#include <View.h>
#include <InterfaceDefs.h>
#include <cstring>
#include <Catalog.h>
#include "NetworkView.h"
#include "Utils.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InterfaceListItem"

class InterfaceListItem : public BListItem {
public:
	using BListItem::Update;
	InterfaceListItem(const BString& name, const BString& type,
		const BString& addr, uint64 sent, uint64 recv,
		uint64 txSpeed, uint64 rxSpeed,
		const BFont* font, NetworkView* view)
		: BListItem(), fGeneration(0), fView(view)
	{
		Update(name, type, addr, sent, recv, txSpeed, rxSpeed, font, true);
	}

	void SetGeneration(int32 generation) { fGeneration = generation; }
	int32 Generation() const { return fGeneration; }
	const BString& Name() const { return fName; }

	void Update(const BString& name, const BString& type, const BString& addr,
		uint64 sent, uint64 recv, uint64 txSpeed, uint64 rxSpeed,
		const BFont* font, bool force = false)
	{
		bool nameChanged    = force || fName    != name;
		bool typeChanged    = force || fType    != type;
		bool addrChanged    = force || fAddr    != addr;
		bool sentChanged    = force || fSent    != sent;
		bool recvChanged    = force || fRecv    != recv;
		bool txChanged      = force || fTxSpeed != txSpeed;
		bool rxChanged      = force || fRxSpeed != rxSpeed;

		fName = name; fType = type; fAddr = addr;
		fSent = sent; fRecv = recv;
		fTxSpeed = txSpeed; fRxSpeed = rxSpeed;

		if (sentChanged) ::FormatBytes(fCachedSent, fSent);
		if (recvChanged) ::FormatBytes(fCachedRecv, fRecv);
		if (txChanged)   fCachedTxSpeed = FormatSpeed(fTxSpeed, 1000000);
		if (rxChanged)   fCachedRxSpeed = FormatSpeed(fRxSpeed, 1000000);

		if (nameChanged) {
			fTruncatedName = fName;
			if (font && fView) {
				float w = std::max(0.0f, fView->NameWidth() - 10.0f);
				font->TruncateString(&fTruncatedName, B_TRUNCATE_END, w);
			}
		}
		if (typeChanged) {
			fTruncatedType = fType;
			if (font && fView) {
				float w = std::max(0.0f, fView->TypeWidth() - 10.0f);
				font->TruncateString(&fTruncatedType, B_TRUNCATE_END, w);
			}
		}
		if (addrChanged) {
			fTruncatedAddr = fAddr;
			if (font && fView) {
				float w = std::max(0.0f, fView->AddrWidth() - 10.0f);
				font->TruncateString(&fTruncatedAddr, B_TRUNCATE_END, w);
			}
		}
	}

	virtual void DrawItem(BView* owner, BRect itemRect, bool complete = false) {
		if (!fView) return;
		rgb_color color = IsSelected()
			? ui_color(B_LIST_SELECTED_BACKGROUND_COLOR)
			: ui_color(B_LIST_BACKGROUND_COLOR);
		owner->SetHighColor(color);
		owner->FillRect(itemRect);

		rgb_color textColor = IsSelected()
			? ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR)
			: ui_color(B_LIST_ITEM_TEXT_COLOR);
		owner->SetHighColor(textColor);

		BFont font;
		owner->GetFont(&font);
		font_height fh;
		font.GetHeight(&fh);

		float x = itemRect.left + 5;
		float y = itemRect.bottom - fh.descent;

		auto drawRight = [&](const BString& str, float width) {
			float w = owner->StringWidth(str.String());
			owner->DrawString(str.String(), BPoint(x + width - w - 5, y));
			x += width;
		};

		owner->DrawString(fTruncatedName.String(), BPoint(x, y)); x += fView->NameWidth();
		owner->DrawString(fTruncatedType.String(), BPoint(x, y)); x += fView->TypeWidth();
		owner->DrawString(fTruncatedAddr.String(), BPoint(x, y)); x += fView->AddrWidth();
		drawRight(fCachedSent,    fView->SentWidth());
		drawRight(fCachedRecv,    fView->RecvWidth());
		drawRight(fCachedTxSpeed, fView->TxSpeedWidth());
		drawRight(fCachedRxSpeed, fView->RxSpeedWidth());
	}

	static bool sSortAscending;

	static int CompareName(const void* a, const void* b) {
		const InterfaceListItem* i1 = *static_cast<const InterfaceListItem* const*>(a);
		const InterfaceListItem* i2 = *static_cast<const InterfaceListItem* const*>(b);
		int result = strcasecmp(i1->fName.String(), i2->fName.String());
		return sSortAscending ? result : -result;
	}
	static int CompareType(const void* a, const void* b) {
		const InterfaceListItem* i1 = *static_cast<const InterfaceListItem* const*>(a);
		const InterfaceListItem* i2 = *static_cast<const InterfaceListItem* const*>(b);
		int result = strcasecmp(i1->fType.String(), i2->fType.String());
		return sSortAscending ? result : -result;
	}
	static int CompareAddr(const void* a, const void* b) {
		const InterfaceListItem* i1 = *static_cast<const InterfaceListItem* const*>(a);
		const InterfaceListItem* i2 = *static_cast<const InterfaceListItem* const*>(b);
		int result = strcasecmp(i1->fAddr.String(), i2->fAddr.String());
		return sSortAscending ? result : -result;
	}
	static int CompareSent(const void* a, const void* b) {
		const InterfaceListItem* i1 = *static_cast<const InterfaceListItem* const*>(a);
		const InterfaceListItem* i2 = *static_cast<const InterfaceListItem* const*>(b);
		int result = 0;
		if (i1->fSent > i2->fSent) result = -1;
		else if (i1->fSent < i2->fSent) result = 1;
		return sSortAscending ? -result : result;
	}
	static int CompareRecv(const void* a, const void* b) {
		const InterfaceListItem* i1 = *static_cast<const InterfaceListItem* const*>(a);
		const InterfaceListItem* i2 = *static_cast<const InterfaceListItem* const*>(b);
		int result = 0;
		if (i1->fRecv > i2->fRecv) result = -1;
		else if (i1->fRecv < i2->fRecv) result = 1;
		return sSortAscending ? -result : result;
	}
	static int CompareTxSpeed(const void* a, const void* b) {
		const InterfaceListItem* i1 = *static_cast<const InterfaceListItem* const*>(a);
		const InterfaceListItem* i2 = *static_cast<const InterfaceListItem* const*>(b);
		int result = 0;
		if (i1->fTxSpeed > i2->fTxSpeed) result = -1;
		else if (i1->fTxSpeed < i2->fTxSpeed) result = 1;
		return sSortAscending ? -result : result;
	}
	static int CompareRxSpeed(const void* a, const void* b) {
		const InterfaceListItem* i1 = *static_cast<const InterfaceListItem* const*>(a);
		const InterfaceListItem* i2 = *static_cast<const InterfaceListItem* const*>(b);
		int result = 0;
		if (i1->fRxSpeed > i2->fRxSpeed) result = -1;
		else if (i1->fRxSpeed < i2->fRxSpeed) result = 1;
		return sSortAscending ? -result : result;
	}
	static int CompareSpeed(const void* a, const void* b) {
		const InterfaceListItem* i1 = *static_cast<const InterfaceListItem* const*>(a);
		const InterfaceListItem* i2 = *static_cast<const InterfaceListItem* const*>(b);
		uint64 s1 = i1->fTxSpeed + i1->fRxSpeed;
		uint64 s2 = i2->fTxSpeed + i2->fRxSpeed;
		int result = 0;
		if (s1 > s2) result = -1;
		else if (s1 < s2) result = 1;
		return sSortAscending ? -result : result;
	}

private:
	BString  fName, fType, fAddr;
	uint64   fSent, fRecv, fTxSpeed, fRxSpeed;
	BString  fCachedSent, fCachedRecv, fCachedTxSpeed, fCachedRxSpeed;
	BString  fTruncatedName, fTruncatedType, fTruncatedAddr;
	int32    fGeneration;
	NetworkView* fView;
};

#endif // INTERFACELISTITEM_H
