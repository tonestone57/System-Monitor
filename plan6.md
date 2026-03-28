1. Modify `SystemDetailsView.h`:
- Add `BStringView* fCPUCoresView;` to the class definition.

2. Modify `SystemDetailsView.cpp`:
- In `_UpdateLabel`:
  - Set alignment to `B_ALIGN_RIGHT, B_ALIGN_VERTICAL_UNSET`.
  - Increase font size by +2.
  - Append a `:` to the label text if it doesn't already have one.
- In `_UpdateSubtext`:
  - Increase font size by +1.
- In `_UpdateText`:
  - Increase font size by +1.
- In the constructor `SystemDetailsView()`:
  - Initialize `fCPUCoresView(NULL)`.
  - Create `fCPULabelView` with `_CreateLabel("cpulabel", B_TRANSLATE("CPU"))` instead of using `_GetCPUCount(&sysInfo)`.
  - Create `fCPUCoresView` with `_CreateSubtext("cpu_cores", _GetCPUCount(&sysInfo))`.
  - Change the layout from `BLayoutBuilder::Group<>(B_VERTICAL)` to use a `BGridLayout`.
  - Add each label to column 0, and its corresponding info view to column 1. Add glue to column 2.
  - The rows will be populated sequentially (e.g., OS on row 0, Kernel on row 1, etc.).
  - For Memory, since there is `fMemUsageView` and `fSwapUsageView`, place `fMemSizeView` in column 0, and place `fMemUsageView` in row N column 1, and `fSwapUsageView` in row N+1 column 1.
  - For CPU, place `fCPULabelView` in row N column 0, and in column 1 place `fCPUInfoView` (row N), `fCPUCoresView` (row N+1), and `fCPUFeaturesView` (row N+2).
  - Add a final empty row with vertical glue to ensure items pack towards the top.

3. Complete Pre commit steps:
- Ensure all mocked tests pass.

4. Submit:
- Submit the changes using the commit message "Condense System Details view into a grid and adjust fonts".
