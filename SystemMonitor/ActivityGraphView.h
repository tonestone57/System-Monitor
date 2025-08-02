#ifndef ACTIVITYGRAPHVIEW_H
#define ACTIVITYGRAPHVIEW_H

#include <View.h>
#include "DataHistory.h"

class BBitmap;

class ActivityGraphView : public BView {
public:
						ActivityGraphView(const char* name, rgb_color color);
	virtual				~ActivityGraphView();

	virtual void		AttachedToWindow();
	virtual void		FrameResized(float width, float height);
	virtual void		Draw(BRect updateRect);

			void		AddValue(bigtime_t time, int64 value);

private:
			void		_UpdateOffscreenBitmap();
			BView*		_OffscreenView();
			void		_DrawHistory();

private:
	rgb_color			fColor;
	BBitmap*			fOffscreen;
	DataHistory*		fHistory;
};

#endif // ACTIVITYGRAPHVIEW_H
