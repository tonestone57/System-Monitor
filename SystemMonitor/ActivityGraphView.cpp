#include "ActivityGraphView.h"
#include <Bitmap.h>
#include <ControlLook.h>

ActivityGraphView::ActivityGraphView(const char* name, rgb_color color)
	: BView(name, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	fColor(color),
	fOffscreen(NULL)
{
	fHistory = new DataHistory(10 * 60000000LL, 1000000);
}


ActivityGraphView::~ActivityGraphView()
{
	delete fOffscreen;
	delete fHistory;
}


void
ActivityGraphView::AttachedToWindow()
{
	BView::AttachedToWindow();
	FrameResized(Bounds().Width(), Bounds().Height());
}


void
ActivityGraphView::FrameResized(float /*width*/, float /*height*/)
{
	_UpdateOffscreenBitmap();
}


void
ActivityGraphView::_UpdateOffscreenBitmap()
{
	BRect frame = Bounds();

	if (fOffscreen != NULL && frame == fOffscreen->Bounds())
		return;

	delete fOffscreen;

	fOffscreen = new(std::nothrow) BBitmap(frame, B_BITMAP_ACCEPTS_VIEWS,
		B_RGB32);
	if (fOffscreen == NULL || fOffscreen->InitCheck() != B_OK) {
		delete fOffscreen;
		fOffscreen = NULL;
		return;
	}

	BView* view = new BView(frame, NULL, B_FOLLOW_NONE, 0);
	fOffscreen->AddChild(view);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	view->SetLowColor(view->ViewColor());
}


BView*
ActivityGraphView::_OffscreenView()
{
	if (fOffscreen == NULL)
		return NULL;

	return fOffscreen->ChildAt(0);
}


void
ActivityGraphView::AddValue(bigtime_t time, int64 value)
{
	fHistory->AddValue(time, value);
	Invalidate();
}


void
ActivityGraphView::Draw(BRect updateRect)
{
	_DrawHistory();
}


void
ActivityGraphView::_DrawHistory()
{
	_UpdateOffscreenBitmap();

	BView* view = this;
	if (fOffscreen != NULL) {
		fOffscreen->Lock();
		view = _OffscreenView();
	}

	BRect frame = Bounds();
	view->FillRect(frame, B_SOLID_LOW);

	uint32 steps = frame.IntegerWidth();
	if (steps <= 0)
		return;

	bigtime_t now = system_time();
	bigtime_t timeStep = 1000000;

	view->SetPenSize(1.5);
	view->SetHighColor(fColor);
	view->SetLineMode(B_BUTT_CAP, B_ROUND_JOIN);
	view->MovePenTo(B_ORIGIN);

	try {
		view->BeginLineArray(steps - 1);
		BPoint prev;
		bool first = true;

		for (uint32 i = 0; i < steps; i++) {
			int64 value = fHistory->ValueAt(now - (steps - 1 - i) * timeStep);
			float y = frame.Height() - (value - fHistory->MinimumValue()) * frame.Height()
				/ (fHistory->MaximumValue() - fHistory->MinimumValue());

			if (first) {
				first = false;
			} else
				view->AddLine(prev, BPoint(i, y), fColor);

			prev.Set(i, y);
		}
		view->EndLineArray();
	} catch (std::bad_alloc&) {
		// ignore
	}

	view->Sync();
	if (fOffscreen != NULL) {
		fOffscreen->Unlock();
		DrawBitmap(fOffscreen, Bounds());
	}
}
