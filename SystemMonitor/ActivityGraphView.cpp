#include "ActivityGraphView.h"
#include <Autolock.h>
#include <Bitmap.h>
#include <ControlLook.h>
#include <Window.h>
#include <Region.h>
#include <algorithm>
#include <new>
#include <cmath>
#include "Utils.h"

ActivityGraphView::ActivityGraphView(const char* name, rgb_color color, color_which systemColor)
	: BView(name, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	fColor(color),
	fSystemColor(systemColor),
	fOffscreen(NULL),
	fResolution(1000000),
	fManualScale(false),
	fManualMin(0),
	fManualMax(0),
	fLastMin(0),
	fLastRange(0),
	fLastRangeValid(false),
	fLastRefresh(0),
	fScrollOffset(0)
{
	fPoints.resize(2048); // Pre-allocate for typical screen widths to avoid reallocations
	fPoints.reserve(4096); // Pre-allocate for typical screen widths (including 4K) to avoid reallocations
	fHistory = new DataHistory(10 * 60000000LL, 1000000);
}


ActivityGraphView::~ActivityGraphView()
{
	delete fOffscreen;
	delete fHistory;
}


void
ActivityGraphView::SetAutoScale()
{
	fManualScale = false;
	Invalidate();
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

				fLastRefresh = 0;
				Invalidate();
			}
			break;
		}
		default:
			BView::MessageReceived(message);
	}
}


void
ActivityGraphView::FrameResized(float width, float /*height*/)
{
	_UpdateOffscreenBitmap();

	// Pre-allocate points vector to avoid frequent resizing during window growth
	size_t needed = static_cast<size_t>(width) + 64;
	if (fPoints.capacity() < needed) {
		fPoints.reserve(std::max(needed, fPoints.capacity() * 2));
	}
	if (fPoints.size() < needed) {
		fPoints.resize(needed);
	}
}


void
ActivityGraphView::_UpdateOffscreenBitmap()
{
	BRect bounds = Bounds();
	bounds.OffsetTo(B_ORIGIN);

	if (fOffscreen != NULL && fOffscreen->Bounds().Contains(bounds)) {
		BView* view = _OffscreenView();
		if (view != NULL && view->Bounds() != bounds) {
			if (fOffscreen->Lock()) {
				view->ResizeTo(bounds.Width(), bounds.Height());
				fOffscreen->Unlock();
			}
			fLastRefresh = 0;
		}
		return;
	}

	delete fOffscreen;
	fOffscreen = NULL;
	fLastRefresh = 0;

	if (Window() == NULL)
		return;

	BAutolock locker(Window());
	if (!locker.IsLocked())
		return;

	// Over-allocate to avoid frequent recreations during resize
	BRect bitmapBounds = bounds;
	bitmapBounds.right += 64;
	bitmapBounds.bottom += 64;

	fOffscreen = new(std::nothrow) BBitmap(bitmapBounds, B_BITMAP_ACCEPTS_VIEWS,
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
ActivityGraphView::SetManualScale(int64 min, int64 max)
{
	fManualScale = true;
	fManualMin = min;
	fManualMax = max;
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

	BView* view = _OffscreenView();
	if (view == NULL)
		return;

	BRect frame = view->Bounds();

	if (fOffscreen->Lock()) {
		uint32 steps = frame.IntegerWidth();
		if (steps > 0) {
			bigtime_t now = system_time();
			bigtime_t timeStep = fResolution;

			bool fullRedraw = true;
			int32 pixelsToScroll = 0;

			if (fLastRefresh > 0) {
				bigtime_t delta = now - fLastRefresh;
				pixelsToScroll = delta / timeStep;

				if (pixelsToScroll < (int32)steps && pixelsToScroll >= 0) {
					 if (pixelsToScroll == 0) {
						 // Optimization: Do nothing if sub-pixel change
						 fullRedraw = false;
					 } else {
						 fullRedraw = false;
					 }
				}
			}

			rgb_color drawColor = fColor;
			if (fSystemColor != (color_which)-1) {
				drawColor = ui_color(fSystemColor);
			}

			rgb_color bg = ui_color(B_PANEL_BACKGROUND_COLOR);
			rgb_color gridColor = tint_color(bg, B_DARKEN_1_TINT);

			int64 min, max;
			if (fManualScale) {
				min = fManualMin;
				max = fManualMax;
			} else {
				min = fHistory->MinimumValue();
				max = fHistory->MaximumValue();
			}
			int64 range = max - min;

			// Force full redraw if scale changed
			if (!fLastRangeValid || min != fLastMin || range != fLastRange) {
				fullRedraw = true;
			}

			if (fullRedraw) {
				fScrollOffset = 0;
				fLastRefresh = now;

				view->SetLowColor(bg);
				view->FillRect(frame, B_SOLID_LOW);

				// Draw Grid
				view->SetDrawingMode(B_OP_COPY);
				view->SetHighColor(gridColor);
				view->SetPenSize(1.0);

				// Horizontal lines
				for (int i = 1; i < 4; i++) {
					float y = frame.top + frame.Height() * i / 4;
					view->StrokeLine(BPoint(frame.left, y), BPoint(frame.right, y));
				}
				// Vertical lines
				BFont viewFont;
				view->GetFont(&viewFont);
				float gridSpacing = 60.0f * GetScaleFactor(&viewFont);
				for (float x = 0; x < frame.Width(); x += gridSpacing) {
					 view->StrokeLine(BPoint(x, frame.top), BPoint(x, frame.bottom));
				}

				// Calculate points for polygon fill and line stroke.
				// points[0] is the bottom-left corner of the fill.
				// points[1...steps] are the actual data points.
				// points[steps+1] is the bottom-right corner of the fill.
				int32 pointCount = steps + 2;

				try {
					if (fPoints.size() < (size_t)pointCount)
						fPoints.resize(pointCount);

					BPoint* points = fPoints.data();

					// Bottom-left corner for polygon fill
					points[0] = BPoint(frame.left, frame.bottom);

					int32 searchIndex = 0;
					for (uint32 i = 0; i < steps; i++) {
						int64 value = fHistory->ValueAt(now - (steps - 1 - i) * timeStep, &searchIndex);
						float y;
						if (range == 0) {
							if (min == 0) y = frame.Height();
							else y = frame.Height() / 2;
						} else
							y = frame.Height() - (value - min) * frame.Height() / range;
						// Offset by 1 to leave room for the bottom-left corner at points[0]
						points[i+1] = BPoint(i, y);
					}
					// Bottom-right corner for polygon fill
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

					if (steps > 1) {
						view->BeginLineArray(steps - 1);
						for (uint32 i = 0; i < steps - 1; i++) {
							view->AddLine(points[i+1], points[i+2], drawColor);
						}
						view->EndLineArray();
					}

					fLastMin = min;
					fLastRange = range;
					fLastRangeValid = true;
				} catch (const std::bad_alloc&) {
					// Ignore update if memory is low
				}
			} else {
				// Partial or sub-pixel Update
				int32 redrawWidth = std::max((int32)1, pixelsToScroll);

				if (pixelsToScroll > 0) {
					// Scroll
					BRect src(pixelsToScroll, 0, frame.right, frame.bottom);
					BRect dst(0, 0, frame.right - pixelsToScroll, frame.bottom);
					view->CopyBits(src, dst);

					BFont viewFont;
					view->GetFont(&viewFont);
					float gridSpacing = 60.0f * GetScaleFactor(&viewFont);
					fScrollOffset += static_cast<float>(pixelsToScroll);
					while (fScrollOffset >= gridSpacing)
						fScrollOffset -= gridSpacing;
					fLastRefresh += static_cast<bigtime_t>(pixelsToScroll) * timeStep;
				}

				// New Area (at least the last pixel)
				BRect newArea(frame.right - redrawWidth, frame.top, frame.right, frame.bottom);

				view->SetLowColor(bg);
				view->FillRect(newArea, B_SOLID_LOW);

				// Draw Grid (New Area)
				view->SetDrawingMode(B_OP_COPY);
				view->SetHighColor(gridColor);
				view->SetPenSize(1.0);

				// Horizontal lines
				for (int i = 1; i < 4; i++) {
					float y = frame.top + frame.Height() * i / 4;
					view->StrokeLine(BPoint(newArea.left, y), BPoint(newArea.right, y));
				}

				// Vertical lines
				BFont viewFont;
				view->GetFont(&viewFont);
				float gridSpacing = 60.0f * GetScaleFactor(&viewFont);
				int64 startK = (int64)ceilf((newArea.left + fScrollOffset) / gridSpacing);
				for (int64 k = startK; ; k++) {
					float x = k * gridSpacing - fScrollOffset;
					if (x > newArea.right) break;
					view->StrokeLine(BPoint(x, frame.top), BPoint(x, frame.bottom));
				}

				// Clip drawing to new area to prevent overlap artifacts
				BRegion clipRegion(newArea);
				view->ConstrainClippingRegion(&clipRegion);

				int32 startI = static_cast<int32>(newArea.left) - 1;
				if (startI < 0) startI = 0;
				int32 endI = steps - 1;
				int32 count = endI - startI + 1;
				// points[0] = start-bottom, points[1...count] = data, points[count+1] = end-bottom
				int32 polyCount = count + 2;

				try {
					if (fPoints.size() < (size_t)polyCount)
						fPoints.resize(polyCount);

					BPoint* points = fPoints.data();

					// Bottom-start corner for partial polygon fill
					points[0] = BPoint(startI, frame.bottom);

					int32 searchIndex = 0;
					for (int32 j = 0; j < count; j++) {
						int32 i = startI + j;
						// For the very last pixel, use 'now' for maximum smoothness
						bigtime_t t;
						if (i == static_cast<int32>(steps) - 1) t = now;
						else t = fLastRefresh - static_cast<bigtime_t>(steps - 1 - i) * timeStep;

						int64 value = fHistory->ValueAt(t, &searchIndex);
						float y;
						if (range == 0) {
							if (min == 0) y = frame.Height();
							else y = frame.Height() / 2;
						} else
							y = frame.Height() - (value - min) * frame.Height() / range;
						// Offset by 1 to leave room for the bottom-start corner at points[0]
						points[j+1] = BPoint(i, y);
					}
					// Bottom-end corner for partial polygon fill
					points[polyCount-1] = BPoint(endI, frame.bottom);

					// Fill
					view->SetDrawingMode(B_OP_ALPHA);
					rgb_color fillColor = drawColor;
					fillColor.alpha = 100;
					view->SetHighColor(fillColor);
					view->FillPolygon(points, polyCount);

					// Stroke
					view->SetDrawingMode(B_OP_COPY);
					view->SetHighColor(drawColor);
					view->SetPenSize(1.5);
					if (count > 1) {
						view->BeginLineArray(count - 1);
						for (int32 j = 0; j < count - 1; j++) {
							view->AddLine(points[j+1], points[j+2], drawColor);
						}
						view->EndLineArray();
					}
				} catch (const std::bad_alloc&) {
					// Ignore
				}

				view->ConstrainClippingRegion(NULL); // Reset clipping
			}
		}
		view->Sync();
		fOffscreen->Unlock();
	}
	DrawBitmap(fOffscreen, frame, Bounds());
}
