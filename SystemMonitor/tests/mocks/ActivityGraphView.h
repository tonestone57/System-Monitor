#ifndef MOCK_ACTIVITY_GRAPH_VIEW_H
#define MOCK_ACTIVITY_GRAPH_VIEW_H

#include "View.h"

class ActivityGraphView : public BView {
public:
    ActivityGraphView(const char* name, rgb_color color, color_which systemColor = (color_which)-1)
        : BView(name, 0) {}
    void SetExplicitMinSize(BSize s) {}
    void SetManualScale(int64_t min, int64_t max) {}
    void AddValue(bigtime_t time, int64_t value) {}
    void SetRefreshInterval(bigtime_t interval) {}
    void SetFillColor(rgb_color color) {}
    void SetDrawGrid(bool drawGrid) {}
    void SetDrawFill(bool drawFill) {}
};

#endif
