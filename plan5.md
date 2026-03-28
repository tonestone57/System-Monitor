Let's check `test_grid.cpp` again for BGridLayout methods.
```cpp
#include <GridLayout.h>
#include <LayoutBuilder.h>
#include <StringView.h>
#include <SpaceLayoutItem.h>
void f() {
    BGridLayout* gridLayout = new BGridLayout(0, 0);
    BLayoutBuilder::Grid<>(gridLayout)
        .Add(new BStringView("",""), 0, 0)
        .Add(BSpaceLayoutItem::CreateGlue(), 0, 1, 2)
        .End();
}
```
Is `Add(BSpaceLayoutItem::CreateGlue(), 0, row + 1, 2)` correct? `BLayoutBuilder::Grid::Add` takes an item, column, row, columnSpan=1, rowSpan=1.
Wait, `Add(BLayoutItem* item, int32 column, int32 row, int32 columnCount = 1, int32 rowCount = 1)`.
So it's `column, row, columnCount, rowCount`.
If column is 0, row is `row + 1`, and columnCount is 2:
`layoutBuilder.Add(BSpaceLayoutItem::CreateGlue(), 0, row + 1, 2);`
Wait, does a `BGridLayout` row expand to fill vertical space if there's no glue?
Usually in Haiku, `BGridLayout` will expand its last row or evenly distribute if weights are unset.
If I just set `gridLayout->SetRowWeight(row + 1, 1.0f)`, the empty row will take the extra space.
Actually, I don't even need to add glue to a `BGridLayout`. I can just add an empty `BView` or use `BSpaceLayoutItem::CreateGlue()`.

Let's test if `BGridLayout` handles `B_USE_DEFAULT_SPACING` and `BLayoutBuilder::Grid` properly in Haiku OS Layout Builder API.
If I look at `MemView.cpp`, it does:
```cpp
	BGridLayout* gridLayout = new BGridLayout(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);

	BLayoutBuilder::Grid<>(gridLayout)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.Add(fTotalMemLabel, 0, 0)
		.Add(fTotalMemValue, 1, 0)
		.Add(BSpaceLayoutItem::CreateGlue(), 2, 0)

		.Add(fUsedMemLabel, 0, 1)
...
	gridLayout->SetColumnWeight(2, 1.0f);
	statsBox->SetLayout(gridLayout);
```
So `MemView.cpp` creates a grid layout, uses `BLayoutBuilder::Grid<>` to populate it, adds a glue item on the last column `2` to push everything left, and sets its weight to `1.0f`.
And for vertical, it seems the last row naturally takes the space, or maybe `BBox` constraints it.

I can do the exact same:
```cpp
	BGridLayout* gridLayout = new BGridLayout(be_control_look->DefaultItemSpacing(), be_control_look->DefaultLabelSpacing());

	auto layoutBuilder = BLayoutBuilder::Grid<>(gridLayout)
		.Add(fVersionLabelView, 0, 0)
		.Add(fVersionInfoView, 1, 0)
        .Add(BSpaceLayoutItem::CreateGlue(), 2, 0) // this column 2 will be the horizontal glue
		.Add(kernelLabel, 0, 1)
		.Add(fKernelDateTimeView, 1, 1)
        ...
```
Then:
```cpp
	gridLayout->SetColumnWeight(2, 1.0f);
	int rowWeight = row + 1;
	layoutBuilder.Add(BSpaceLayoutItem::CreateGlue(), 0, rowWeight, 3);
	gridLayout->SetRowWeight(rowWeight, 1.0f);
```

Let's check `SystemDetailsView::SystemDetailsView()` to see how fonts are configured.
```cpp
void SystemDetailsView::_UpdateLabel(BStringView* label)
{
	label->SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT, B_ALIGN_VERTICAL_UNSET));
	BFont font(be_bold_font);
	font.SetSize(font.Size() + 2);
	label->SetFont(&font, B_FONT_ALL);
	label->SetHighColor(139, 0, 0, 255);
	BString text = label->Text();
	text.ToUpper();
	if (!text.EndsWith(":")) {
		text << ":";
	}
	label->SetText(text.String());
}
```

And `_UpdateSubtext`:
```cpp
void SystemDetailsView::_UpdateSubtext(BStringView* subtext)
{
	subtext->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_UNSET));
	BFont font(be_plain_font);
	font.SetSize(font.Size() + 1);
	subtext->SetFont(&font, B_FONT_ALL);
	subtext->SetHighColor(0, 0, 0, 255);
}
```
Wait, `fCPUFeaturesView` and `fUptimeView` are `BTextView`s.
```cpp
void SystemDetailsView::_UpdateText(BTextView* textView)
{
	textView->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_TOP));
	rgb_color black = {0, 0, 0, 255};
	BFont font(be_plain_font);
	font.SetSize(font.Size() + 1);
	textView->SetFontAndColor(&font, B_FONT_ALL, &black);
    ...
```

Let's refine the plan and submit for review.
