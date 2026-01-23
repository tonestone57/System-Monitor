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

	virtual void		AttachedToWindow();
	virtual void		MessageReceived(BMessage* message);
	virtual void		FrameResized(float width, float height);
	virtual void		Draw(BRect updateRect);

			void		AddValue(bigtime_t time, int64 value);
			void		SetRefreshInterval(bigtime_t interval);

private:
			void		_UpdateOffscreenBitmap();
			BView*		_OffscreenView();
			void		_DrawHistory();

private:
	rgb_color			fColor;
    color_which         fSystemColor;
	BBitmap*			fOffscreen;
	DataHistory*		fHistory;
	bigtime_t			fResolution;
	std::vector<BPoint>	fPoints;
};

#endif // ACTIVITYGRAPHVIEW_H
