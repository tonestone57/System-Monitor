#ifndef LIVEGRAPHVIEW_H
#define LIVEGRAPHVIEW_H

#include <View.h>
#include <deque>
#include <algorithm>

class LiveGraphView : public BView {
public:
    LiveGraphView(const char* name,
                  rgb_color color = {0, 150, 0, 255},
                  size_t maxSamples = 100)
        : BView(name, B_WILL_DRAW | B_PULSE_NEEDED),
          fColor(color),
          fMaxSamples(maxSamples)
    {
        SetViewColor(B_TRANSPARENT_COLOR);
    }

    void AddSample(float percent) {
        if (fHistory.size() >= fMaxSamples)
            fHistory.pop_front();
        fHistory.push_back(std::clamp(percent, 0.0f, 100.0f));
        Invalidate();
    }

    virtual void Draw(BRect updateRect) override {
        if (fHistory.size() < 2)
            return;

        SetHighColor(fColor);
        BRect bounds = Bounds();
        float width = bounds.Width();
        float height = bounds.Height();
        float dx = width / (fMaxSamples - 1);

        BPoint prev(0, height * (1.0f - fHistory[0] / 100.0f));
        for (size_t i = 1; i < fHistory.size(); ++i) {
            float x = i * dx;
            float y = height * (1.0f - fHistory[i] / 100.0f);
            StrokeLine(prev, BPoint(x, y));
            prev.Set(x, y);
        }
    }

private:
	rgb_color fColor;
    const size_t fMaxSamples;
    std::deque<float> fHistory;    
};

#endif // LIVEGRAPHVIEW_H
