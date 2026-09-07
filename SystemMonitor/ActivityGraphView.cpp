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
#include <vector>

ActivityGraphView::ActivityGraphView(const char* name, rgb_color color, color_which systemColor)
	: BView(name, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_FRAME_EVENTS | B_SUPPORTS_LAYOUT),
	fColor(color),
	fSystemColor(systemColor),
	fOffscreen(NULL),
	fResolution(1000000),
	fFillColor({0,0,0,0}),
	fDrawGrid(true),
	fDrawFill(true),
	fManualScale(false),
	fManualMin(0),
	fManualMax(0),
	fLastMin(0),
	fLastRange(0),
	fLastRangeValid(false),
	fLastRefresh(0),
	fScrollOffset(0)
{
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
	fLastRangeValid = false;
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
				if (deltaY > 0) {
					fResolution *= 2;
				} else if (deltaY < 0) {
					fResolution /= 2;
				} else {
					break;
				}

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

	float safeWidth = (width > 0.0f) ? width : 0.0f;
	// Pre-allocate points vector to avoid frequent reallocations during window growth
	size_t needed = static_cast<size_t>(safeWidth) + 128;
	if (fPoints.capacity() < needed) {
		fPoints.reserve(std::max(needed, fPoints.capacity() * 2));
	}
	if (fPoints.size() < static_cast<size_t>(needed))
		fPoints.resize(needed);
}


void
ActivityGraphView::_UpdateOffscreenBitmap()
{
	BRect bounds = Bounds();
	bounds.OffsetTo(B_ORIGIN);

	if (fOffscreen != NULL && fOffscreen->Bounds().Contains(bounds)) {
		BView* view = _OffscreenView();
		if (view != NULL) {
			if (fOffscreen->Lock()) {
				if (view->Bounds() != bounds) {
					view->ResizeTo(bounds.Width(), bounds.Height());
					fLastRefresh = 0;
				}
				fOffscreen->Unlock();
			}
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
	if (fHistory)
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
	if (min > max)
		std::swap(min, max);
	fManualMin = min;
	fManualMax = max;
	fLastRangeValid = false;
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

	bool locked = fOffscreen->Lock();
	BRect viewBounds;
	if (locked) {
		viewBounds = view->Bounds();
		BRect frame = viewBounds;

		float frameWidth = frame.Width() > 0.0f ? frame.Width() : 0.0f;
		uint32 steps = static_cast<uint32>(frameWidth) + 1;
		if (steps > 0) {
			bigtime_t now = system_time();
			bigtime_t timeStep = fResolution;

			bool fullRedraw = true;
			int32 pixelsToScroll = 0;

			if (fLastRefresh > 0) {
				bigtime_t delta = now - fLastRefresh;
				pixelsToScroll = delta / timeStep;

				if (pixelsToScroll < (int32)steps && pixelsToScroll >= 0)
					fullRedraw = false;
			}

			// Force a full redraw if we don't have enough history yet to scroll
			// or if we just started receiving data.
			if (fHistory == NULL || fHistory->Start() == fHistory->End()) {
				fullRedraw = true;
			}

			rgb_color drawColor = fColor;
			if (fSystemColor != (color_which)-1) {
				drawColor = ui_color(fSystemColor);
			}

			rgb_color bg = ViewColor();
			rgb_color gridColor = tint_color(bg, B_DARKEN_1_TINT);

			int64 min, max;
			if (fManualScale) {
				min = fManualMin;
				max = fManualMax;
			} else if (fHistory != NULL) {
				min = fHistory->MinimumValue();
				max = fHistory->MaximumValue();
			} else {
				min = 0;
				max = 100;
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
				if (fDrawGrid) {
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
				}

				// Calculate points for polygon fill and line stroke.
				// points[0] is the bottom-left corner of the fill.
				// points[1...steps] are the actual data points.
				// points[steps+1] is the bottom-right corner of the fill.
				int32 pointCount = steps + 2;

				try {
					if (fPoints.capacity() < static_cast<size_t>(pointCount))
						fPoints.reserve(pointCount + 64);
					if (fPoints.size() < static_cast<size_t>(pointCount))
						fPoints.resize(pointCount);

					BPoint* points = fPoints.data();

					// Bottom-left corner for polygon fill
					points[0] = BPoint(frame.left, frame.bottom);

					std::vector<int64> values(steps, 0);
					if (fHistory != NULL)
						fHistory->GetValues(values.data(), steps, now - (steps - 1) * timeStep, timeStep);

					for (uint32 i = 0; i < steps; i++) {
						int64 value = values[i];
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
					if (fDrawFill) {
						view->SetDrawingMode(B_OP_ALPHA);
						rgb_color fillColor = drawColor;
						fillColor.alpha = 100;
						if (fFillColor.alpha != 0) {
							fillColor = fFillColor;
						}
						view->SetHighColor(fillColor);
						view->FillPolygon(points, pointCount);
					}

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
				if (fDrawGrid) {
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
					int64 startK = (int64)ceilf((newArea.left - (frame.right - fScrollOffset)) / gridSpacing);
					for (int64 k = startK; ; k++) {
						float x = frame.right - fScrollOffset + k * gridSpacing;
						if (x > newArea.right) break;
						if (x >= newArea.left)
							view->StrokeLine(BPoint(x, frame.top), BPoint(x, frame.bottom));
					}
				}

				// Clip drawing to new area to prevent overlap artifacts
				BRegion clipRegion(newArea);
				view->ConstrainClippingRegion(&clipRegion);

				int32 startI = static_cast<int32>(newArea.left) - 1;
				if (startI < 0) startI = 0;
				int32 endI = steps - 1;
				int32 count = endI - startI + 1;

				if (count > 0) {
					// points[0] = start-bottom, points[1...count] = data, points[count+1] = end-bottom
					int32 polyCount = count + 2;

					try {
						if (fPoints.capacity() < static_cast<size_t>(polyCount))
							fPoints.reserve(polyCount + 64);
						if (fPoints.size() < static_cast<size_t>(polyCount))
							fPoints.resize(polyCount);

						BPoint* points = fPoints.data();

						// Bottom-start corner for partial polygon fill
						points[0] = BPoint(startI, frame.bottom);

						std::vector<int64> values(count, 0);
						if (fHistory != NULL) {
							bigtime_t offset = (static_cast<bigtime_t>(steps - 1) - static_cast<bigtime_t>(startI)) * timeStep;
							fHistory->GetValues(values.data(), count, fLastRefresh - offset, timeStep);
						}

						for (int32 j = 0; j < count; j++) {
							int32 i = startI + j;
							// For the very last pixel, use 'now' for maximum smoothness
							bigtime_t t;
							int64 value;
							if (i == static_cast<int32>(steps) - 1 && fHistory != NULL) {
								t = now;
								value = fHistory->ValueAt(t, NULL);
							} else {
								value = values[j];
							}

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
						if (fDrawFill) {
							view->SetDrawingMode(B_OP_ALPHA);
							rgb_color fillColor = drawColor;
							fillColor.alpha = 100;
							if (fFillColor.alpha != 0) {
								fillColor = fFillColor;
							}
							view->SetHighColor(fillColor);
							view->FillPolygon(points, polyCount);
						}

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
				}

				view->ConstrainClippingRegion(NULL); // Reset clipping
			}
		}
		view->Sync();
		fOffscreen->Unlock();
	}

	if (locked && view != NULL) {
		DrawBitmap(fOffscreen, viewBounds, Bounds());
	}
}

void
ActivityGraphView::SetFillColor(rgb_color color)
{
	fFillColor = color;
	Invalidate();
}

void
ActivityGraphView::SetDrawGrid(bool drawGrid)
{
	fDrawGrid = drawGrid;
	Invalidate();
}

void
ActivityGraphView::SetDrawFill(bool drawFill)
{
	fDrawFill = drawFill;
	Invalidate();
}
