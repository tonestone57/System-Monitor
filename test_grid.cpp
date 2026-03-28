#include <GridLayout.h>
#include <LayoutBuilder.h>
#include <StringView.h>
#include <Box.h>
void f() {
    BBox* statsBox = new BBox("MemoryStatsBox");
    BGridLayout* gridLayout = new BGridLayout(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
    BLayoutBuilder::Grid<>(gridLayout)
        .Add(new BStringView("",""), 0, 0)
        .Add(new BStringView("",""), 1, 0)
        .End();
}
