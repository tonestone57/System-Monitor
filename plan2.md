I need to rewrite `_UpdateLabel` to make font size +2:
```cpp
void SystemDetailsView::_UpdateLabel(BStringView* label)
{
	label->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_UNSET));
	BFont font(be_bold_font);
	font.SetSize(font.Size() + 2);
	label->SetFont(&font, B_FONT_ALL);
	label->SetHighColor(139, 0, 0, 255);
	BString text = label->Text();
	text.ToUpper();
	text << ":";
	label->SetText(text.String());
}
```

Wait, but `text.ToUpper();` makes it all caps.
Wait, "Change the format by putting information on the same line as category heading. For instance, put - GPU: VMware SVGA, BATTERY: Unknown, etc. on same line instead of on separate lines to condense the information closer together."

So in `SystemDetailsView::SystemDetailsView()`, where it lays everything out:
Currently:
```cpp
	auto layoutBuilder = BLayoutBuilder::Group<>(detailsGroup, B_VERTICAL)
		// OS Version:
		.Add(fVersionLabelView)
		.Add(fVersionInfoView)
		.AddStrut(offset)
```
We should probably do:
```cpp
		.AddGroup(B_HORIZONTAL)
			.Add(fVersionLabelView)
			.Add(fVersionInfoView)
			.AddGlue()
		.End()
		.AddStrut(offset)
```

Wait, if we use a BGridLayout, they will all align nicely!
```cpp
	BGridLayout* gridLayout = new BGridLayout(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
	auto layoutBuilder = BLayoutBuilder::Grid<>(gridLayout)
		// OS Version:
		.Add(fVersionLabelView, 0, 0)
		.Add(fVersionInfoView, 1, 0)
		// Kernel:
		.Add(kernelLabel, 0, 1)
		.Add(fKernelDateTimeView, 1, 1)
		...
```
Ah wait, if they are horizontally close together, maybe they just want them grouped horizontally?
"For instance, put - GPU: VMware SVGA, BATTERY: Unknown, etc. on same line instead of on separate lines to condense the information closer together."
If we just group them horizontally:
```cpp
	auto layoutBuilder = BLayoutBuilder::Group<>(detailsGroup, B_VERTICAL)
		.AddGroup(B_HORIZONTAL, B_USE_ITEM_SPACING)
			.Add(fVersionLabelView)
			.Add(fVersionInfoView)
			.AddGlue()
		.End()
```
Actually, using a grid layout `BGridLayout` is very elegant for this and aligns the colons. Let's see what is standard.
But wait! Some things like CPU features are a `BTextView` that spans multiple lines. "Move 6 logical cores down under the actual cpu brand and model."

If we use a BGridLayout:
Row 0: OS, OS_info
Row 1: Kernel, Kernel_info
Row 2: Time running, Uptime_info
Row 3: Packages, Packages_info
Row 4: Shell, Shell_info
Row 5: Resolution, Resolution_info
Row 6: DE, DE_info
Row 7: WM, WM_info
Row 8: Font, Font_info
Row 9: CPU, CPU_info (brand and model)
Row 10: (empty label), CPU_cores_info (6 logical cores)
Row 11: (empty label), CPU_features (BTextView)
Row 12: GPU, GPU_info
Row 13: Memory, Mem_info
Row 14: (empty label), Swap_info
Row 15: Disk, Disk_info
Row 16: Local IP, IP_info
Row 17: Battery, Battery_info
Row 18: Locale, Locale_info

Wait, what about the colon? "For instance, put - GPU: VMware SVGA". I'll just change `_CreateLabel` to add a colon at the end:
```cpp
	BString text = label->Text();
	text.ToUpper();
	text << ":";
	label->SetText(text.String());
```
Wait, the prompt says "put - GPU: VMware SVGA, BATTERY: Unknown, etc. on same line instead of on separate lines".
Does it literally mean put a hyphen `- `? Or is that just a bullet point in their sentence? "For instance, put - GPU: VMware SVGA...". That implies it's just an example of format: `GPU: VMware SVGA`.

Let's test grid layout compilation in the mock. The mock does not have BGridLayout or BLayoutBuilder::Grid... wait!
Earlier I tried `test_grid.cpp` and it failed because `GridLayout.h` didn't exist in the Haiku headers?! Wait!
In Haiku, it's `<GridLayout.h>` and `<LayoutBuilder.h>`.
Wait, `test_grid.cpp` failed in the mock tests environment or the root environment? I ran it with `g++ -c test_grid.cpp -I/boot/system/...`. That directory is for Haiku OS. We are on a Linux sandbox! There is no `/boot/system`!
