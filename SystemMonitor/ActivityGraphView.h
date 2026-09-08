#ifndef ACTIVITYGRAPHVIEW_H
#define ACTIVITYGRAPHVIEW_H

#include <View.h>
#include <vector>
#include "DataHistory.h"

class BBitmap;

class ActivityGraphView : public BView {
public:
						ActivityGraphView(const char* name, rgb_color color, color_which systemColor = (color_which)-1);
	virtual				~ActivityGraphView();

	virtual void		AttachedToWindow() override;
	virtual void		MessageReceived(BMessage* message) override;
	virtual void		FrameResized(float width, float height) override;
	virtual void		Draw(BRect updateRect) override;

			void		AddValue(bigtime_t time, int64 value);
			void		SetRefreshInterval(bigtime_t interval);
			void		SetManualScale(int64 min, int64 max);
			void		SetAutoScale();

			void		SetFillColor(rgb_color color);
			void		SetDrawGrid(bool drawGrid);
			void		SetDrawFill(bool drawFill);

private:
			void		_UpdateOffscreenBitmap();
			BView*		_OffscreenView();
			void		_DrawHistory();

private:
	rgb_color			fColor;
	color_which		 fSystemColor;
	BBitmap*			fOffscreen;
	DataHistory*		fHistory;
	bigtime_t			fResolution;
	std::vector<BPoint>	fPoints;

	rgb_color			fFillColor;
	bool				fDrawGrid;
	bool				fDrawFill;
	bool				fManualScale;
	int64				fManualMin;
	int64				fManualMax;

	int64				fLastMin;
	int64				fLastRange;
	bool				fLastRangeValid;

	bigtime_t			fLastRefresh;
	float				fScrollOffset;
};

#endif // ACTIVITYGRAPHVIEW_H
