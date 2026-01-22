#include "ActivityGraphView.h"
#include <Autolock.h>
#include <Bitmap.h>
#include <ControlLook.h>
#include <Window.h>
#include <new>

ActivityGraphView::ActivityGraphView(const char* name, rgb_color color, color_which systemColor)
	: BView(name, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	fColor(color),
    fSystemColor(systemColor),
	fOffscreen(NULL),
	fResolution(1000000)
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
ActivityGraphView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_MOUSE_WHEEL_CHANGED: {
			float deltaY;
			if (message->FindFloat("be:wheel_delta_y", &deltaY) == B_OK) {
				if (deltaY > 0)
					fResolution *= 2;
				else
					fResolution /= 2;

				if (fResolution < 10000) fResolution = 10000;
				if (fResolution > 60000000) fResolution = 60000000;

				Invalidate();
			}
			break;
		}
		default:
			BView::MessageReceived(message);
	}
}


void
ActivityGraphView::FrameResized(float /*width*/, float /*height*/)
{
	_UpdateOffscreenBitmap();
}


void
ActivityGraphView::_UpdateOffscreenBitmap()
{
	BRect bounds = Bounds();
	bounds.OffsetTo(B_ORIGIN);

	if (fOffscreen != NULL && bounds == fOffscreen->Bounds())
		return;

	delete fOffscreen;
	fOffscreen = NULL;

	if (Window() == NULL)
		return;

	BAutolock locker(Window());
	if (!locker.IsLocked())
		return;

	fOffscreen = new(std::nothrow) BBitmap(bounds, B_BITMAP_ACCEPTS_VIEWS,
		B_RGB32);
	if (fOffscreen == NULL || fOffscreen->InitCheck() != B_OK) {
		delete fOffscreen;
		fOffscreen = NULL;
		return;
	}

	BView* view = new(std::nothrow) BView(bounds, NULL, B_FOLLOW_NONE, 0);
	if (view == NULL) {
		delete fOffscreen;
		fOffscreen = NULL;
		return;
	}
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
ActivityGraphView::SetRefreshInterval(bigtime_t interval)
{
	if (fHistory)
		fHistory->SetRefreshInterval(interval);
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
			bigtime_t timeStep = fResolution;

            rgb_color drawColor = fColor;
            if (fSystemColor != (color_which)-1) {
                drawColor = ui_color(fSystemColor);
            }

            // Draw Grid
            view->SetDrawingMode(B_OP_COPY);
            rgb_color gridColor = tint_color(view->LowColor(), B_DARKEN_1_TINT);
            view->SetHighColor(gridColor);
            view->SetPenSize(1.0);

            // Horizontal lines
            for (int i = 1; i < 4; i++) {
                float y = frame.top + frame.Height() * i / 4;
                view->StrokeLine(BPoint(frame.left, y), BPoint(frame.right, y));
            }
            // Vertical lines
            for (int x = 0; x < frame.Width(); x += 60) {
                 view->StrokeLine(BPoint(x, frame.top), BPoint(x, frame.bottom));
            }

			int64 min = fHistory->MinimumValue();
			int64 max = fHistory->MaximumValue();
			int64 range = max - min;

            // Calculate points
            int32 pointCount = steps + 2;

            try {
                if (fPoints.size() < (size_t)pointCount)
                    fPoints.resize(pointCount);

                BPoint* points = fPoints.data();

                points[0] = BPoint(frame.left, frame.bottom);

                for (uint32 i = 0; i < steps; i++) {
                    int64 value = fHistory->ValueAt(now - (steps - 1 - i) * timeStep);
                    float y;
                    if (range == 0)
                        y = frame.Height() / 2;
                    else
                        y = frame.Height() - (value - min) * frame.Height() / range;
                    points[i+1] = BPoint(i, y);
                }
                points[pointCount-1] = BPoint(frame.right, frame.bottom);

                // Fill
                view->SetDrawingMode(B_OP_ALPHA);
                rgb_color fillColor = drawColor;
                fillColor.alpha = 100;
                view->SetHighColor(fillColor);
                view->FillPolygon(points, pointCount);

                // Stroke Line
                view->SetDrawingMode(B_OP_COPY);
                view->SetHighColor(drawColor);
                view->SetPenSize(1.5);

                view->BeginLineArray(steps - 1);
                for (uint32 i = 0; i < steps - 1; i++) {
                    view->AddLine(points[i+1], points[i+2], drawColor);
                }
                view->EndLineArray();
            } catch (const std::bad_alloc&) {
                // Ignore update if memory is low
            }
		}
		view->Sync();
		fOffscreen->Unlock();
	}
	DrawBitmap(fOffscreen, Bounds());
}
