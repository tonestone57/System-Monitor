Wait, if I change `fCPULabelView` to `CPU` and swap the order, here's what the constructor layout should look like:
Instead of `BLayoutBuilder::Group<>(detailsGroup, B_VERTICAL)`, I should use `BGridLayout` with two columns.
Column 0 is labels, Column 1 is values.
```cpp
	BGridLayout* gridLayout = new BGridLayout(B_USE_DEFAULT_SPACING, 0);
	auto layoutBuilder = BLayoutBuilder::Grid<>(gridLayout)
		// OS Version:
		.Add(fVersionLabelView, 0, 0)
		.Add(fVersionInfoView, 1, 0)
		// Kernel:
		.Add(kernelLabel, 0, 1)
		.Add(fKernelDateTimeView, 1, 1)
		// Time running:
		.Add(uptimeLabel, 0, 2)
		.Add(fUptimeView, 1, 2)
        // Packages
		.Add(fPackagesLabelView, 0, 3)
		.Add(fPackagesInfoView, 1, 3)
        // Shell
		.Add(fShellLabelView, 0, 4)
		.Add(fShellInfoView, 1, 4)
        // Resolution
		.Add(displayLabel, 0, 5)
		.Add(fDisplayInfoView, 1, 5)
        // DE
		.Add(fDELabelView, 0, 6)
		.Add(fDEInfoView, 1, 6)
        // WM
		.Add(fWMLabelView, 0, 7)
		.Add(fWMInfoView, 1, 7)
        // Font
		.Add(fFontLabelView, 0, 8)
		.Add(fFontInfoView, 1, 8)
        // CPU
		.Add(fCPULabelView, 0, 9)
		.Add(fCPUInfoView, 1, 9)
		.Add(fCPUCoresView, 1, 10)
		.Add(fCPUFeaturesView, 1, 11)
        // GPU
		.Add(gpuLabel, 0, 12)
		.Add(fGPUInfoView, 1, 12)
        // Memory
		.Add(fMemSizeView, 0, 13)
		.AddGroup(B_VERTICAL, 0, 1, 13)
			.Add(fMemUsageView)
			.Add(fSwapUsageView)
		.End()
        // Disk
		.Add(diskLabel, 0, 14)
		.Add(fDiskUsageView, 1, 14)
        // IP
		.Add(fIPLabelView, 0, 15)
		.Add(fIPInfoView, 1, 15)
		// Battery
        // Locale...
```
Wait, the prompt says "Change the format by putting information on the same line as category heading... Make category heading font size +2 bigger. Make information font +1 bigger."
Also "Move 6 logical cores down under the actual cpu brand and model."

If I use `BGridLayout` it will look like this:
OS:                   Haiku (x86_gcc2)
KERNEL:               Oct 15 2021 15:30:20
...

Wait, the prompt says "For instance, put - GPU: VMware SVGA, BATTERY: Unknown, etc. on same line instead of on separate lines to condense the information closer together."
Currently they are laid out using:
```cpp
		.Add(gpuLabel)
		.Add(fGPUInfoView)
```
Since it's a `B_VERTICAL` group, they are stacked!
```
GPU
VMware SVGA
```
Ah! So "on same line" means literally horizontally beside each other!
Instead of:
```
GPU:
VMware SVGA
```
They want:
```
GPU: VMware SVGA
```

So yes, using `BGridLayout` OR horizontal groups is the answer!
Wait, but if I use horizontal groups, they won't align as a table. Does the user want a table or just a string `GPU: VMware SVGA`?
If they just want it on the same line, either `BGridLayout` or horizontal groups works. `BGridLayout` aligns the colon nicely and looks professional, like `MemView.cpp` does it.
Let's see if `SystemDetailsView` currently uses colons.
```cpp
void SystemDetailsView::_UpdateLabel(BStringView* label)
{
	label->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_UNSET));
	label->SetFont(be_bold_font, B_FONT_FAMILY_AND_STYLE);
	label->SetHighColor(139, 0, 0, 255);
	BString text = label->Text();
	text.ToUpper();
	label->SetText(text.String());
}
```
Currently there are no colons. Wait, if I change it to `BGridLayout`, do I need to add colons? The user example:
"For instance, put - GPU: VMware SVGA, BATTERY: Unknown, etc. on same line instead of on separate lines to condense the information closer together."
So they want me to add a colon!

Let's modify `_UpdateLabel` to add a colon:
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
	text << ":";
	label->SetText(text.String());
}
```
Wait, if I use `BGridLayout` and set alignment to `B_ALIGN_RIGHT` for labels and `B_ALIGN_LEFT` for values, it will look like:
```
         GPU: VMware SVGA
     BATTERY: Unknown
 6 LOGICAL CORES: ...  <-- Wait, the prompt says "Change 6 LOGICAL CORES to CPU."
```

Yes, "Change 6 LOGICAL CORES to CPU. Move 6 logical cores down under the actual cpu brand and model."
So for CPU:
Label: `CPU:`
Value row 1: `Intel(R) Core(TM) i7-8750H CPU @ 2.20GHz`
Value row 2: `6 Logical Cores`
Value row 3: `Features: fpu vme de pse tsc msr ...` (this is `fCPUFeaturesView`)

Wait, if we use `BLayoutBuilder::Group<>(B_VERTICAL)` and then for each item `BLayoutBuilder::Group<>(B_HORIZONTAL).Add(label).Add(info).AddGlue()`, the colon will be right next to the label.
```
GPU: VMware SVGA
BATTERY: Unknown
```
Whereas in `BGridLayout`:
```
      GPU: VMware SVGA
  BATTERY: Unknown
```
Which one did they mean? "GPU: VMware SVGA ... on same line instead of on separate lines to condense the information closer together."
I will just use `BGridLayout`. It's standard for key-value views in Haiku. I'll make the labels `B_ALIGN_LEFT` though, or `B_ALIGN_RIGHT`? `SystemDetailsView` previously had them left-aligned. If I left-align them in a grid layout:
```
GPU:     VMware SVGA
BATTERY: Unknown
```
Wait, if I add glue after the info, the info will hug the labels if the columns are tight, or spacing will be fixed.
If I use `BGridLayout` it is simpler to manage. Wait, if I do `BGridLayout`, the first column width will be the width of the longest label (e.g., `RESOLUTION:` or `DISK USAGE (ROOT):`).
Let's see: `DISK USAGE (ROOT):` is long.
If they just want it closer together, a horizontal group for each row might actually condense them horizontally as well.
Let's use a `BGridLayout` but make the columns hug their content. `gridLayout->SetColumnWeight(1, 1.0f);` for the second column (values), and first column weight 0.

Wait! A very important thing about Font size:
"Make category heading font size +2 bigger. Make information font +1 bigger."

So in `_UpdateLabel`:
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
And in `_UpdateSubtext`:
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
And `_UpdateText`:
```cpp
void SystemDetailsView::_UpdateText(BTextView* textView)
{
	textView->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_TOP));
	rgb_color black = {0, 0, 0, 255};
	BFont font(be_plain_font);
	font.SetSize(font.Size() + 1);
	textView->SetFontAndColor(&font, B_FONT_ALL, &black);
    // ...
```
