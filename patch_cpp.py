import re

with open("SystemMonitor/SystemDetailsView.cpp", "r") as f:
    content = f.read()

# 1. Update constructor init list
content = content.replace(
    "fCPULabelView(NULL),\n\t  fCPUInfoView(NULL),\n\t  fCPUFeaturesView(NULL),",
    "fCPULabelView(NULL),\n\t  fCPUInfoView(NULL),\n\t  fCPUCoresView(NULL),\n\t  fCPUFeaturesView(NULL),"
)

# 2. Update fCPULabelView creation
content = content.replace(
    'fCPULabelView = _CreateLabel("cpulabel", _GetCPUCount(&sysInfo));\n\tfCPUInfoView = _CreateSubtext("cputext", _GetCPUInfo());',
    'fCPULabelView = _CreateLabel("cpulabel", B_TRANSLATE("CPU"));\n\tfCPUInfoView = _CreateSubtext("cputext", _GetCPUInfo());\n\tfCPUCoresView = _CreateSubtext("cpu_cores", _GetCPUCount(&sysInfo));'
)

# 3. Update BLayoutBuilder layout
old_layout_str = """BGroupView* detailsGroup = new BGroupView(B_VERTICAL);
	detailsGroup->SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR);

	auto layoutBuilder = BLayoutBuilder::Group<>(detailsGroup, B_VERTICAL)
		// OS Version:
		.Add(fVersionLabelView)
		.Add(fVersionInfoView)
		.AddStrut(offset)
		// Kernel:
		.Add(kernelLabel)
		.Add(fKernelDateTimeView)
		.AddStrut(offset)
		// Time running:
		.Add(uptimeLabel)
		.Add(fUptimeView)
		.AddStrut(offset)
		// Packages:
		.Add(fPackagesLabelView)
		.Add(fPackagesInfoView)
		.AddStrut(offset)
		// Shell:
		.Add(fShellLabelView)
		.Add(fShellInfoView)
		.AddStrut(offset)
		// Resolution:
		.Add(displayLabel)
		.Add(fDisplayInfoView)
		.AddStrut(offset)
		// DE:
		.Add(fDELabelView)
		.Add(fDEInfoView)
		.AddStrut(offset)
		// WM:
		.Add(fWMLabelView)
		.Add(fWMInfoView)
		.AddStrut(offset)
		// Font:
		.Add(fFontLabelView)
		.Add(fFontInfoView)
		.AddStrut(offset)
		// CPU / Processors:
		.Add(fCPULabelView)
		.Add(fCPUInfoView)
		.Add(fCPUFeaturesView)
		.AddStrut(offset)
		// GPU:
		.Add(gpuLabel)
		.Add(fGPUInfoView)
		.AddStrut(offset)
		// Memory:
		.Add(fMemSizeView)
		.Add(fMemUsageView)
		.Add(fSwapUsageView)
		.AddStrut(offset)
		// Disk:
		.Add(diskLabel)
		.Add(fDiskUsageView)
		.AddStrut(offset)
		// Local IP:
		.Add(fIPLabelView)
		.Add(fIPInfoView)
		.AddStrut(offset);

	if (fBatteryLabelView) {
		layoutBuilder.Add(fBatteryLabelView)
			.Add(fBatteryInfoView)
			.AddStrut(offset);
	}

	layoutBuilder.Add(fLocaleLabelView)
		.Add(fLocaleInfoView)
		.AddGlue()
		.SetInsets(inset)
		.End();"""

new_layout_str = """BGridLayout* gridLayout = new BGridLayout(be_control_look->DefaultItemSpacing(), be_control_look->DefaultItemSpacing() / 2);
	BGroupView* detailsGroup = new BGroupView(B_VERTICAL);
	detailsGroup->SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR);
	detailsGroup->SetLayout(gridLayout);

	auto layoutBuilder = BLayoutBuilder::Grid<>(gridLayout)
		// OS Version:
		.Add(fVersionLabelView, 0, 0)
		.Add(fVersionInfoView, 1, 0)
		.Add(BSpaceLayoutItem::CreateGlue(), 2, 0)
		// Kernel:
		.Add(kernelLabel, 0, 1)
		.Add(fKernelDateTimeView, 1, 1)
		// Time running:
		.Add(uptimeLabel, 0, 2)
		.Add(fUptimeView, 1, 2)
		// Packages:
		.Add(fPackagesLabelView, 0, 3)
		.Add(fPackagesInfoView, 1, 3)
		// Shell:
		.Add(fShellLabelView, 0, 4)
		.Add(fShellInfoView, 1, 4)
		// Resolution:
		.Add(displayLabel, 0, 5)
		.Add(fDisplayInfoView, 1, 5)
		// DE:
		.Add(fDELabelView, 0, 6)
		.Add(fDEInfoView, 1, 6)
		// WM:
		.Add(fWMLabelView, 0, 7)
		.Add(fWMInfoView, 1, 7)
		// Font:
		.Add(fFontLabelView, 0, 8)
		.Add(fFontInfoView, 1, 8)
		// CPU / Processors:
		.Add(fCPULabelView, 0, 9)
		.Add(fCPUInfoView, 1, 9)
		.Add(fCPUCoresView, 1, 10)
		.Add(fCPUFeaturesView, 1, 11)
		// GPU:
		.Add(gpuLabel, 0, 12)
		.Add(fGPUInfoView, 1, 12)
		// Memory:
		.Add(fMemSizeView, 0, 13)
		.Add(fMemUsageView, 1, 13)
		.Add(fSwapUsageView, 1, 14)
		// Disk:
		.Add(diskLabel, 0, 15)
		.Add(fDiskUsageView, 1, 15)
		// Local IP:
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

	row++;
	layoutBuilder.Add(BSpaceLayoutItem::CreateGlue(), 0, row, 3);
	gridLayout->SetRowWeight(row, 1.0f);
	gridLayout->SetColumnWeight(2, 1.0f);

	layoutBuilder.SetInsets(inset)
		.End();"""

if old_layout_str in content:
    content = content.replace(old_layout_str, new_layout_str)
else:
    print("WARNING: Could not find old_layout_str")

# 4. Update _UpdateLabel
old_update_label = """void SystemDetailsView::_UpdateLabel(BStringView* label)
{
	label->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_UNSET));
	label->SetFont(be_bold_font, B_FONT_FAMILY_AND_STYLE);
	label->SetHighColor(139, 0, 0, 255);
	BString text = label->Text();
	text.ToUpper();
	label->SetText(text.String());
}"""

new_update_label = """void SystemDetailsView::_UpdateLabel(BStringView* label)
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
}"""

if old_update_label in content:
    content = content.replace(old_update_label, new_update_label)
else:
    print("WARNING: Could not find old_update_label")

# 5. Update _UpdateSubtext
old_update_subtext = """void SystemDetailsView::_UpdateSubtext(BStringView* subtext)
{
	subtext->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_UNSET));
	subtext->SetFont(be_plain_font, B_FONT_FAMILY_AND_STYLE);
	subtext->SetHighColor(0, 0, 0, 255);
}"""

new_update_subtext = """void SystemDetailsView::_UpdateSubtext(BStringView* subtext)
{
	subtext->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_UNSET));
	BFont font(be_plain_font);
	font.SetSize(font.Size() + 1);
	subtext->SetFont(&font, B_FONT_ALL);
	subtext->SetHighColor(0, 0, 0, 255);
}"""

if old_update_subtext in content:
    content = content.replace(old_update_subtext, new_update_subtext)
else:
    print("WARNING: Could not find old_update_subtext")

# 6. Update _UpdateText
old_update_text = """void SystemDetailsView::_UpdateText(BTextView* textView)
{
	textView->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_TOP));
	rgb_color black = {0, 0, 0, 255};
	textView->SetFontAndColor(be_plain_font, B_FONT_FAMILY_AND_STYLE, &black);"""

new_update_text = """void SystemDetailsView::_UpdateText(BTextView* textView)
{
	textView->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_TOP));
	rgb_color black = {0, 0, 0, 255};
	BFont font(be_plain_font);
	font.SetSize(font.Size() + 1);
	textView->SetFontAndColor(&font, B_FONT_ALL, &black);"""

if old_update_text in content:
    content = content.replace(old_update_text, new_update_text)
else:
    print("WARNING: Could not find old_update_text")

with open("SystemMonitor/SystemDetailsView.cpp", "w") as f:
    f.write(content)
