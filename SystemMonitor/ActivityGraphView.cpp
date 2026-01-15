#include "ActivityGraphView.h"
#include <Autolock.h>
#include <Bitmap.h>
#include <ControlLook.h>
#include <Window.h>

ActivityGraphView::ActivityGraphView(const char* name, rgb_color color, color_which systemColor)
	: BView(name, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	fColor(color),
    fSystemColor(systemColor),
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
	if (fOffscreen != NULL && Bounds() == fOffscreen->Bounds())
		return;

	delete fOffscreen;
	fOffscreen = NULL;

	if (Window() == NULL)
		return;

	BAutolock locker(Window());
	if (!locker.IsLocked())
		return;

	fOffscreen = new(std::nothrow) BBitmap(Bounds(), B_BITMAP_ACCEPTS_VIEWS,
		B_RGB32);
	if (fOffscreen == NULL || fOffscreen->InitCheck() != B_OK) {
		delete fOffscreen;
		fOffscreen = NULL;
		return;
	}

	BView* view = new BView(Bounds(), NULL, B_FOLLOW_NONE, 0);
	fOffscreen->AddChild(view);
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
	if (fOffscreen == NULL)
		_UpdateOffscreenBitmap();

	if (fOffscreen == NULL)
		return;

	if (fOffscreen->Lock()) {
		BView* view = _OffscreenView();
		BRect frame = view->Bounds();
		view->SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		view->FillRect(frame, B_SOLID_LOW);

		uint32 steps = frame.IntegerWidth();
		if (steps > 0) {
			bigtime_t now = system_time();
			bigtime_t timeStep = 1000000;

			view->SetPenSize(1.5);

            rgb_color drawColor = fColor;
            if (fSystemColor != (color_which)-1) {
                drawColor = ui_color(fSystemColor);
            }

			view->SetHighColor(drawColor);
			view->SetLineMode(B_BUTT_CAP, B_ROUND_JOIN);
			view->MovePenTo(B_ORIGIN);

			view->BeginLineArray(steps - 1);
			BPoint prev;
			bool first = true;
			int64 min = fHistory->MinimumValue();
			int64 max = fHistory->MaximumValue();
			int64 range = max - min;

			for (uint32 i = 0; i < steps; i++) {
				int64 value = fHistory->ValueAt(now - (steps - 1 - i) * timeStep);
				float y;
				if (range == 0)
					y = frame.Height() / 2;
				else {
					y = frame.Height() - (value - min) * frame.Height()
						/ range;
				}

				if (first) {
					first = false;
				} else
					view->AddLine(prev, BPoint(i, y), drawColor);

				prev.Set(i, y);
			}
			view->EndLineArray();
		}
		view->Sync();
		fOffscreen->Unlock();
	}
	DrawBitmap(fOffscreen, Bounds());
}
