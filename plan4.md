Wait, if `fCPULabelView` originally had text like "6 LOGICAL CORES", now it should be "CPU".
And `fCPUCoresView` should be a subtext showing "6 Logical Cores".

Let's look at `SystemDetailsView::_GetCPUCount(system_info* sysInfo)`:
```cpp
BString SystemDetailsView::_GetCPUCount(system_info* sysInfo)
{
	static BStringFormat format(B_TRANSLATE_COMMENT(
		"{0, plural, one{# Logical Core} other{# Logical Cores}}",
		"\"1 Logical Core\" or \"6 Logical Cores\""));

	BString processorLabel;
	format.Format(processorLabel, sysInfo->cpu_count);
	return processorLabel;
}
```
Currently `fCPULabelView` is created like this:
`fCPULabelView = _CreateLabel("cpulabel", _GetCPUCount(&sysInfo));`
This causes the label text to be the returned value, and it will be capitalized, and then a colon appended: "6 LOGICAL CORES:" !
We need to change it:
```cpp
	fCPULabelView = _CreateLabel("cpulabel", B_TRANSLATE("CPU"));
	fCPUCoresView = _CreateSubtext("cpu_cores", _GetCPUCount(&sysInfo));
	fCPUInfoView = _CreateSubtext("cputext", _GetCPUInfo());
```

And layout:
```cpp
	BGridLayout* gridLayout = new BGridLayout(B_USE_DEFAULT_SPACING, 0); // 0 vertical spacing to "condense the information closer together"

	auto layoutBuilder = BLayoutBuilder::Grid<>(gridLayout)
		// OS Version:
		.Add(fVersionLabelView, 0, 0)
		.Add(fVersionInfoView, 1, 0)
		.Add(BSpaceLayoutItem::CreateVerticalStrut(offset), 0, 1, 2)
		// Kernel:
		.Add(kernelLabel, 0, 2)
		.Add(fKernelDateTimeView, 1, 2)
		.Add(BSpaceLayoutItem::CreateVerticalStrut(offset), 0, 3, 2)
        // ...
```
Wait, if the user explicitly wants them "on the same line instead of on separate lines to condense the information closer together", using a `BGridLayout` with vertical spacing 0 might make the lines TOO close? No, `BStringView` has its own text height. If we add `BSpaceLayoutItem::CreateVerticalStrut(offset)` between rows, it'll retain some spacing. The main point is moving the text from vertically stacked to side-by-side.

Let's do this:
Instead of rewriting `detailsGroup` to be a `BGridLayout`, I can just use a `BGridLayout` for the whole thing. BUT `BLayoutBuilder::Group<>(detailsGroup, B_VERTICAL)` was originally used.
```cpp
	BGridLayout* gridLayout = new BGridLayout(be_control_look->DefaultItemSpacing(), be_control_look->DefaultItemSpacing() / 2);
	BGroupView* detailsGroup = new BGroupView(B_VERTICAL);
	detailsGroup->SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR);

	// Add grid layout to the details group
	detailsGroup->SetLayout(gridLayout);
```

Let's do:
```cpp
	auto layoutBuilder = BLayoutBuilder::Grid<>(gridLayout)
		.Add(fVersionLabelView, 0, 0)
		.Add(fVersionInfoView, 1, 0)
		.Add(kernelLabel, 0, 1)
		.Add(fKernelDateTimeView, 1, 1)
		.Add(uptimeLabel, 0, 2)
		.Add(fUptimeView, 1, 2)
		.Add(fPackagesLabelView, 0, 3)
		.Add(fPackagesInfoView, 1, 3)
		.Add(fShellLabelView, 0, 4)
		.Add(fShellInfoView, 1, 4)
		.Add(displayLabel, 0, 5)
		.Add(fDisplayInfoView, 1, 5)
		.Add(fDELabelView, 0, 6)
		.Add(fDEInfoView, 1, 6)
		.Add(fWMLabelView, 0, 7)
		.Add(fWMInfoView, 1, 7)
		.Add(fFontLabelView, 0, 8)
		.Add(fFontInfoView, 1, 8)
		.Add(fCPULabelView, 0, 9)
		.Add(fCPUInfoView, 1, 9)
		.Add(fCPUCoresView, 1, 10)
		.Add(fCPUFeaturesView, 1, 11)
		.Add(gpuLabel, 0, 12)
		.Add(fGPUInfoView, 1, 12)
		.Add(fMemSizeView, 0, 13)
		.Add(fMemUsageView, 1, 13)
		.Add(fSwapUsageView, 1, 14)
		.Add(diskLabel, 0, 15)
		.Add(fDiskUsageView, 1, 15)
		.Add(fIPLabelView, 0, 16)
		.Add(fIPInfoView, 1, 16);

	int row = 17;
	if (fBatteryLabelView) {
		layoutBuilder.Add(fBatteryLabelView, 0, row)
			.Add(fBatteryInfoView, 1, row);
		row++;
	}

	layoutBuilder.Add(fLocaleLabelView, 0, row)
		.Add(fLocaleInfoView, 1, row);

    // Add glue row
    layoutBuilder.Add(BSpaceLayoutItem::CreateGlue(), 0, row + 1, 2);

	layoutBuilder.SetInsets(inset)
		.End();
```
