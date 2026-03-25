import sys

# Patch Header
header_file = "SystemMonitor/NetworkView.h"
with open(header_file, "r") as f:
    content = f.read()

content = content.replace(
    "std::unordered_map<BString, InterfaceListItem*, BStringHash> fInterfaceItemMap;",
    "std::unordered_map<BString, InterfaceListItem*, BStringHash> fInterfaceItemMap; // Maps interface name to list item for O(1) lookup"
)

with open(header_file, "w") as f:
    f.write(content)

# Patch CPP
cpp_file = "SystemMonitor/NetworkView.cpp"
with open(cpp_file, "r") as f:
    content = f.read()

old_func = """void NetworkView::_RestoreSelection(const BString& selectedName)
{
	if (selectedName.IsEmpty())
		return;

	auto it = fInterfaceItemMap.find(selectedName);
	if (it != fInterfaceItemMap.end()) {
		int32 index = fInterfaceListView->IndexOf(it->second);
		if (index >= 0) {
			fInterfaceListView->Select(index);
		}
	}
}"""

new_func = """void NetworkView::_RestoreSelection(const BString& selectedName)
{
	if (selectedName.IsEmpty())
		return;

	// O(1) lookup to find the target item, avoiding O(N) string comparisons.
	// We still do an O(N) pointer comparison loop to find the BListView index,
	// but pointer comparisons are virtually instantaneous compared to string ops.
	auto it = fInterfaceItemMap.find(selectedName);
	if (it != fInterfaceItemMap.end()) {
		InterfaceListItem* target = it->second;
		int32 count = fInterfaceListView->CountItems();
		for (int32 i = 0; i < count; i++) {
			if (static_cast<InterfaceListItem*>(fInterfaceListView->ItemAt(i)) == target) {
				fInterfaceListView->Select(i);
				break;
			}
		}
	}
}"""

if old_func in content:
    content = content.replace(old_func, new_func)
    with open(cpp_file, "w") as f:
        f.write(content)
    print("Successfully patched both files.")
else:
    print("Could not find the old function in NetworkView.cpp")
