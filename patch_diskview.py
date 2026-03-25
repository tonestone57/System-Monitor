import sys

cpp_file = "SystemMonitor/DiskView.cpp"
with open(cpp_file, "r") as f:
    content = f.read()

old_func = """void DiskView::_RestoreSelection(dev_t selectedID)
{
	if (selectedID == -1)
		return;

	for (int32 i = 0; i < fDiskListView->CountItems(); i++) {
		DiskListItem* item = static_cast<DiskListItem*>(fDiskListView->ItemAt(i));
		if (item && item->DeviceID() == selectedID) {
			fDiskListView->Select(i);
			break;
		}
	}
}"""

new_func = """void DiskView::_RestoreSelection(dev_t selectedID)
{
	if (selectedID == -1)
		return;

	auto it = fDeviceItemMap.find(selectedID);
	if (it != fDeviceItemMap.end()) {
		DiskListItem* target = it->second;
		int32 count = fDiskListView->CountItems();
		for (int32 i = 0; i < count; i++) {
			if (static_cast<DiskListItem*>(fDiskListView->ItemAt(i)) == target) {
				fDiskListView->Select(i);
				break;
			}
		}
	}
}"""

if old_func in content:
    content = content.replace(old_func, new_func)
    with open(cpp_file, "w") as f:
        f.write(content)
    print("Successfully patched DiskView.cpp")
else:
    print("Could not find the old function in DiskView.cpp")
