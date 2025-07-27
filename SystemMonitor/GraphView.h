#ifndef GRAPHVIEW_H
#define GRAPHVIEW_H

#include <View.h>
#include <deque>
#include <algorithm>

class GraphView : public BView {
public:
    GraphView(const char* name,
              rgb_color color = {0, 150, 0, 255},
              size_t maxSamples = 100);

    void AddSample(float percent);

    virtual void Draw(BRect updateRect) override;

private:
    rgb_color fColor;
    const size_t fMaxSamples;
    std::deque<float> fHistory;
};

#endif // GRAPHVIEW_H
