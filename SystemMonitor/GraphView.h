#ifndef GRAPHVIEW_H
#define GRAPHVIEW_H

#include <View.h>
#include <deque>
#include <algorithm>
#include <Bitmap.h>

class GraphView : public BView {
public:
    GraphView(const char* name,
              rgb_color color = {0, 150, 0, 255},
              size_t maxSamples = 100);
    virtual ~GraphView();

    void AddSample(float percent);

    virtual void Draw(BRect updateRect) override;
    virtual void FrameResized(float newWidth, float newHeight) override;
    virtual void AttachedToWindow() override;

private:
    void _Render();
    void _InitBitmap();

    BBitmap* fBitmap;
    BView* fOffscreenView;
    rgb_color fColor;
    const size_t fMaxSamples;
    std::deque<float> fHistory;
    BPoint fPreviousPoint;
};

#endif // GRAPHVIEW_H
