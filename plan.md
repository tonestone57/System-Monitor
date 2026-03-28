1. Modify `SystemDetailsView.h`:
- Remove `fCPULabelView` and `fCPUInfoView`.
- Instead of using `fCPULabelView` (which had "6 Logical Cores") and `fCPUInfoView` (which had brand and model), we will have:
    - A category label view for CPU (with "CPU").
    - A subtext view for brand and model.
    - A subtext view for logical cores under it.

Wait, actually, I want to change the layout significantly: "Change the format by putting information on the same line as category heading. For instance, put - GPU: VMware SVGA, BATTERY: Unknown, etc. on same line instead of on separate lines to condense the information closer together."
And: "Make category heading font size +2 bigger. Make information font +1 bigger."

Let's look at what `_CreateLabel` and `_CreateSubtext` do currently.

Wait, I need to read the prompt carefully.
Prompt: "For System tab: Change 6 LOGICAL CORES to CPU. Move 6 logical cores down under the actual cpu brand and model. Change the format by putting information on the same line as category heading. For instance, put - GPU: VMware SVGA, BATTERY: Unknown, etc. on same line instead of on separate lines to condense the information closer together. Make category heading font size +2 bigger. Make information font +1 bigger."
