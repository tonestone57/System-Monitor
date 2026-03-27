#ifndef MOCK_LIST_ITEM_H
#define MOCK_LIST_ITEM_H

#include "../HaikuMocks.h"
#include "View.h"

class BListItem {
public:
	BListItem(uint32 outlineLevel = 0, bool expanded = false)
		: fSelected(false) {}
	virtual ~BListItem() {}

	bool IsSelected() const { return fSelected; }
	void Select() { fSelected = true; }
	void Deselect() { fSelected = false; }

	virtual void DrawItem(BView* owner, BRect itemRect, bool complete = false) = 0;
	virtual void Update(BView* owner, const BFont* font) {}

private:
	bool fSelected;
};

#endif
