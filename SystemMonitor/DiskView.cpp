#include "DiskView.h"
#include "Utils.h"
#include <LayoutBuilder.h>
#include <StringView.h>
#include <OS.h>
#include <cstdio>
#include <cstring>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>
#include <VolumeRoster.h>
#include <fs_info.h>
#include <ListView.h>
#include <ListItem.h>
#include <Box.h>
#include <Font.h>
#include <Messenger.h>
#include <Catalog.h>
#include <ScrollView.h>
#include <vector>
#include "DiskListItem.h"

bool DiskListItem::sSortAscending = true;

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DiskView"

const float kBaseDiskDeviceWidth = 120;
const float kBaseDiskMountWidth = 120;
const float kBaseDiskFSWidth = 80;
const float kBaseDiskTotalWidth = 100;
const float kBaseDiskUsedWidth = 100;
const float kBaseDiskFreeWidth = 100;
const float kBaseDiskPercentWidth = 80;


DiskView::DiskView()
	: BView("DiskView", B_WILL_DRAW | B_SUPPORTS_LAYOUT),
	  fUpdateThread(-1),
	  fScanSem(-1),
	  fTerminated(false),
	  fPerformanceViewVisible(true),
	  fRefreshInterval(1000000),
	  fListGeneration(0),
	  fTotalCapValue(NULL),
	  fTotalUsedValue(NULL),
	  fTotalFreeValue(NULL),
	  fRootVolValue(NULL),
	  fLastSumTotal(UINT64_MAX),
	  fLastSumUsed(UINT64_MAX),
	  fLastSumFree(UINT64_MAX),
	  fSortMode(SORT_DISK_BY_DEVICE),
	  fSortAscending(true)
{
	SetViewColor(ui_color(B_DOCUMENT_BACKGROUND_COLOR));
	fScanSem = create_sem(0, "disk scan sem");

	// Summary Box
	fTotalCapValue = new BStringView("total_cap_val", "N/A");
	fTotalUsedValue = new BStringView("total_used_val", "N/A");
	fTotalFreeValue = new BStringView("total_free_val", "N/A");
	fRootVolValue = new BStringView("root_vol_val", "N/A");

	BBox* summaryBox = new BBox("DiskSummaryBox");
	summaryBox->SetLabel(B_TRANSLATE("Disk Summary"));

	BGridLayout* summaryGrid = new BGridLayout(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
	summaryGrid->SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);

	summaryGrid->AddView(new BStringView(NULL, B_TRANSLATE("Total Storage:")), 0, 0);
	summaryGrid->AddView(fTotalCapValue, 1, 0);

	summaryGrid->AddView(new BStringView(NULL, B_TRANSLATE("Used Space:")), 2, 0);
	summaryGrid->AddView(fTotalUsedValue, 3, 0);

	summaryGrid->AddView(new BStringView(NULL, B_TRANSLATE("Free Space:")), 0, 1);
	summaryGrid->AddView(fTotalFreeValue, 1, 1);

	summaryGrid->AddView(new BStringView(NULL, B_TRANSLATE("Root Disk:")), 2, 1);
	summaryGrid->AddView(fRootVolValue, 3, 1);

	summaryGrid->SetColumnWeight(1, 1.0f);
	summaryGrid->SetColumnWeight(3, 1.0f);
	summaryBox->SetLayout(summaryGrid);

	// Calculate scaling
	BFont font;
	GetFont(&font);
	float scale = GetScaleFactor(&font);

	fDeviceWidth = kBaseDiskDeviceWidth * scale;
	fMountWidth = kBaseDiskMountWidth * scale;
	fFSWidth = kBaseDiskFSWidth * scale;
	fTotalWidth = kBaseDiskTotalWidth * scale;
	fUsedWidth = kBaseDiskUsedWidth * scale;
	fFreeWidth = kBaseDiskFreeWidth * scale;
	fPercentWidth = kBaseDiskPercentWidth * scale;

	// Header view
	BGroupView* headerView = new BGroupView(B_HORIZONTAL, 0);
	headerView->SetViewColor(ui_color(B_DOCUMENT_BACKGROUND_COLOR));
	BLayoutBuilder::Group<>(headerView).SetInsets(5, 0, 0, 0);

	auto addHeader = [&](const char* label, float width, int32 mode, alignment align = B_ALIGN_LEFT) {
		ClickableHeaderView* sv = new ClickableHeaderView(label, width, mode, this);
		sv->SetAlignment(align);
		headerView->AddChild(sv);
		fHeaders.push_back(sv);
	};

	addHeader(B_TRANSLATE("Device"), fDeviceWidth, SORT_DISK_BY_DEVICE);
	addHeader(B_TRANSLATE("Mount Point"), fMountWidth, SORT_DISK_BY_MOUNT);
	addHeader(B_TRANSLATE("FS Type"), fFSWidth, SORT_DISK_BY_FS);
	addHeader(B_TRANSLATE("Total"), fTotalWidth, SORT_DISK_BY_TOTAL, B_ALIGN_RIGHT);
	addHeader(B_TRANSLATE("Used"), fUsedWidth, SORT_DISK_BY_USED, B_ALIGN_RIGHT);
	addHeader(B_TRANSLATE("Free"), fFreeWidth, SORT_DISK_BY_FREE, B_ALIGN_RIGHT);
	addHeader(B_TRANSLATE("Activity"), fPercentWidth, SORT_DISK_BY_PERCENT, B_ALIGN_CENTER);

	BLayoutBuilder::Group<>(headerView).AddGlue();

	headerView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 20 * scale));

	fDiskListView = new BListView("disk_list", B_SINGLE_SELECTION_LIST, B_WILL_DRAW | B_NAVIGABLE);
	BScrollView* diskScrollView = new BScrollView("disk_scroll", fDiskListView, 0, false, true);
	diskScrollView->SetBorder(B_NO_BORDER);

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(summaryBox)
		.Add(headerView)
		.Add(diskScrollView)
	.End();
}

DiskView::~DiskView()
{
	fTerminated = true;
	if (fScanSem >= 0) {
		delete_sem(fScanSem);
		fScanSem = -1;
	}
	if (fUpdateThread >= 0) {
		status_t dummy;
		wait_for_thread(fUpdateThread, &dummy);
		fUpdateThread = -1;
	}

	fDiskListView->MakeEmpty();
	fDeviceItemMap.clear();
}

void DiskView::AttachedToWindow()
{
	BView::AttachedToWindow();
	fTerminated = false;

	if (fScanSem < 0)
		fScanSem = create_sem(0, "disk scan sem");

	BVolumeRoster().StartWatching(BMessenger(this));
	// Initial scan to populate cache
	_ScanVolumes();

	fUpdateThread = spawn_thread(UpdateThread, "DiskView Update", B_NORMAL_PRIORITY, this);
	if (fUpdateThread >= 0)
		resume_thread(fUpdateThread);
}

void DiskView::DetachedFromWindow()
{
	fTerminated = true;
	BVolumeRoster().StopWatching();
	if (fScanSem >= 0) {
		delete_sem(fScanSem);
		fScanSem = -1;
	}
	if (fUpdateThread >= 0) {
		status_t dummy;
		wait_for_thread(fUpdateThread, &dummy);
		fUpdateThread = -1;
	}
	BView::DetachedFromWindow();
}

void DiskView::MessageReceived(BMessage* message)
{
	if (message->what == kMsgDiskDataUpdate) {
		UpdateData(message);
	} else if (message->what == MSG_HEADER_CLICKED) {
		int32 mode;
		if (message->FindInt32("mode", &mode) == B_OK) {
			if (fSortMode == (DiskSortMode)mode) {
				fSortAscending = !fSortAscending;
			} else {
				fSortMode = (DiskSortMode)mode;
				fSortAscending = (fSortMode == SORT_DISK_BY_DEVICE || fSortMode == SORT_DISK_BY_MOUNT || fSortMode == SORT_DISK_BY_FS);
			}
			_SortItems();
			fDiskListView->Invalidate();
		}
	} else if (message->what == B_NODE_MONITOR) {
		int32 opcode;
		if (message->FindInt32("opcode", &opcode) == B_OK) {
			if (opcode == B_DEVICE_MOUNTED) {
				dev_t device;
				if (message->FindInt32("new_device", &device) == B_OK || message->FindInt32("device", &device) == B_OK) {
					BVolume volume(device);
					if (volume.InitCheck() == B_OK && volume.Capacity() > 0) {
						DiskInfo info;
						if (GetDiskInfo(volume, info) == B_OK) {
							if (fLocker.Lock()) {
								fVolumeCache[info.deviceID] = info;
								fLocker.Unlock();
							}
							if (fScanSem >= 0)
								release_sem(fScanSem);
						}
					}
				}
			} else if (opcode == B_DEVICE_UNMOUNTED) {
				dev_t device;
				if (message->FindInt32("device", &device) == B_OK) {
					if (fLocker.Lock()) {
						fVolumeCache.erase(device);
						fLocker.Unlock();
					}
					if (fScanSem >= 0)
						release_sem(fScanSem);
				}
			}
		}
	} else {
		BView::MessageReceived(message);
	}
}

void DiskView::SetRefreshInterval(bigtime_t interval)
{
	if (interval <= 0) return;
	fRefreshInterval = interval;
	if (fScanSem >= 0)
		release_sem(fScanSem);
}

status_t DiskView::GetDiskInfo(BVolume& volume, DiskInfo& info) {
	fs_info fsInfo;
	status_t status = fs_stat_dev(volume.Device(), &fsInfo);
	if (status != B_OK) {
		return status;
	}

	info.deviceID = fsInfo.dev;
	info.totalSize = static_cast<uint64>(fsInfo.total_blocks) * static_cast<uint64>(fsInfo.block_size);
	info.freeSize = static_cast<uint64>(fsInfo.free_blocks) * static_cast<uint64>(fsInfo.block_size);
	info.fileSystemType = fsInfo.fsh_name;

	BDirectory mountDir;
	status = volume.GetRootDirectory(&mountDir);
	if (status != B_OK) {
		return status;
	}
	BEntry mountEntry;
	status = mountDir.GetEntry(&mountEntry);
	if (status != B_OK) {
		return status;
	}
	BPath mountPath;
	status = mountEntry.GetPath(&mountPath);
	if (status != B_OK) {
		return status;
	}
	info.mountPoint = (mountPath.Path() != NULL) ? mountPath.Path() : "";

	char volumeName[B_FILE_NAME_LENGTH];
	if (volume.GetName(volumeName) == B_OK && strlen(volumeName) > 0) {
		info.deviceName = volumeName;
	} else {
		info.deviceName = fsInfo.device_name;
	}
	return B_OK;
}

int32 DiskView::UpdateThread(void* data)
{
	DiskView* view = static_cast<DiskView*>(data);
	BMessenger target(view);

	std::vector<dev_t> volumesToPoll;

	while (!view->fTerminated) {
		status_t err = acquire_sem_etc(view->fScanSem, 1, B_RELATIVE_TIMEOUT, view->fRefreshInterval);
		if (err != B_OK && err != B_TIMED_OUT && err != B_INTERRUPTED)
			break;

		if (view->fTerminated) break;

		if (!view->fPerformanceViewVisible)
			continue;

		// Drain the semaphore if we were woken up explicitly (e.g. interval change)
		if (err == B_OK) {
			int32 count;
			if (get_sem_count(view->fScanSem, &count) == B_OK && count > 0)
				acquire_sem_etc(view->fScanSem, count, B_RELATIVE_TIMEOUT, 0);
		}

		BMessage updateMsg(kMsgDiskDataUpdate);

		volumesToPoll.clear();
		{
			BAutolock locker(view->fLocker);
			if (locker.IsLocked()) {
				volumesToPoll.reserve(view->fVolumeCache.size());
				for (const auto& pair : view->fVolumeCache) {
					 volumesToPoll.push_back(pair.first);
				}
			}
		}

		for (auto dev : volumesToPoll) {
			fs_info fsInfo;
			if (fs_stat_dev(dev, &fsInfo) != B_OK)
				continue;

			uint64 totalSize = static_cast<uint64>(fsInfo.total_blocks) * static_cast<uint64>(fsInfo.block_size);
			uint64 freeSize = static_cast<uint64>(fsInfo.free_blocks) * static_cast<uint64>(fsInfo.block_size);
			const char* deviceName = (strlen(fsInfo.volume_name) > 0) ? fsInfo.volume_name : fsInfo.device_name;

			{
				BAutolock locker(view->fLocker);
				if (locker.IsLocked()) {
					auto it = view->fVolumeCache.find(dev);
					if (it != view->fVolumeCache.end()) {
						it->second.totalSize = totalSize;
						it->second.freeSize = freeSize;
						it->second.deviceName = deviceName;

						const DiskInfo& info = it->second;

						BMessage volMsg;
						volMsg.AddInt32("device_id", static_cast<int32>(info.deviceID));
						volMsg.AddString("device_name", info.deviceName);
						volMsg.AddString("mount_point", info.mountPoint);
						volMsg.AddString("fs_type", info.fileSystemType);
						volMsg.AddUInt64("total_size", info.totalSize);
						volMsg.AddUInt64("free_size", info.freeSize);

						updateMsg.AddMessage("volume", &volMsg);
					}
				}
			}
		}

		target.SendMessage(&updateMsg);
	}
	return B_OK;
}

void DiskView::UpdateData(BMessage* message)
{
	BAutolock locker(fLocker);
	if (!locker.IsLocked())
		return;

	if (!fDiskListView) {
		return;
	}

	// Preserve selection
	int32 selection = fDiskListView->CurrentSelection();
	dev_t selectedID = -1;
	if (selection >= 0) {
		DiskListItem* item = static_cast<DiskListItem*>(fDiskListView->ItemAt(selection));
		if (item) selectedID = item->DeviceID();
	}

	fListGeneration++;
	int32 count = 0;
	type_code type;
	message->GetInfo("volume", &type, &count);

	// Get Font once
	BFont font;
	fDiskListView->GetFont(&font);

	bool fontChanged = (font != fCachedFont);
	if (fontChanged) {
		fCachedFont = font;
		float scale = GetScaleFactor(&font);
		fDeviceWidth = kBaseDiskDeviceWidth * scale;
		fMountWidth = kBaseDiskMountWidth * scale;
		fFSWidth = kBaseDiskFSWidth * scale;
		fTotalWidth = kBaseDiskTotalWidth * scale;
		fUsedWidth = kBaseDiskUsedWidth * scale;
		fFreeWidth = kBaseDiskFreeWidth * scale;
		fPercentWidth = kBaseDiskPercentWidth * scale;

		UpdateHeaderWidths(fHeaders, { fDeviceWidth, fMountWidth, fFSWidth, fTotalWidth, fUsedWidth, fFreeWidth, fPercentWidth });
	}

	for (int32 i = 0; i < count; i++) {
		BMessage volMsg;
		if (message->FindMessage("volume", i, &volMsg) != B_OK) continue;

		int32 deviceID;
		if (volMsg.FindInt32("device_id", &deviceID) != B_OK) continue;

		BString deviceName = volMsg.FindString("device_name");
		BString mountPoint = volMsg.FindString("mount_point");
		BString fsType = volMsg.FindString("fs_type");
		uint64 totalSize = 0, freeSize = 0;
		volMsg.FindUInt64("total_size", &totalSize);
		volMsg.FindUInt64("free_size", &freeSize);

		uint64 usedSize = (totalSize >= freeSize) ? totalSize - freeSize : 0;
		double usagePercent = 0.0;
		if (totalSize > 0) {
			usagePercent = static_cast<double>(usedSize) / totalSize * 100.0;
			if (usagePercent < 0.0) usagePercent = 0.0;
			if (usagePercent > 100.0) usagePercent = 100.0;
		}

		DiskListItem* item;
		auto result = fDeviceItemMap.emplace(deviceID, nullptr);
		if (result.second) {
			auto newItem = std::unique_ptr<DiskListItem>(new DiskListItem(deviceID, deviceName, mountPoint, fsType, totalSize, usedSize, freeSize, usagePercent, &font, this));
			item = newItem.get();
			fDiskListView->AddItem(item);
			result.first->second = std::move(newItem);
		} else {
			item = result.first->second.get();
			item->Update(deviceName, mountPoint, fsType, totalSize, usedSize, freeSize, usagePercent, &font, fontChanged);
		}
		item->SetGeneration(fListGeneration);
	}

	for (auto it = fDeviceItemMap.begin(); it != fDeviceItemMap.end();) {
		if (it->second->Generation() != fListGeneration) {
			DiskListItem* item = it->second.get();
			fDiskListView->RemoveItem(item);
			it = fDeviceItemMap.erase(it);
		} else {
			++it;
		}
	}
	_SortItems();

	_RestoreSelection(selectedID);

	// Update Disk Summary Box
	uint64 sumTotal = 0, sumUsed = 0, sumFree = 0;
	for (const auto& pair : fVolumeCache) {
		sumTotal += pair.second.totalSize;
		sumFree += pair.second.freeSize;
		if (pair.second.totalSize >= pair.second.freeSize)
			sumUsed += (pair.second.totalSize - pair.second.freeSize);
	}
	if (fTotalCapValue && (sumTotal != fLastSumTotal || sumUsed != fLastSumUsed || sumFree != fLastSumFree)) {
		fLastSumTotal = sumTotal;
		fLastSumUsed = sumUsed;
		fLastSumFree = sumFree;
		BString capStr, usedStr, freeStr;
		FormatBytes(capStr, sumTotal);
		FormatBytes(usedStr, sumUsed);
		FormatBytes(freeStr, sumFree);
		fTotalCapValue->SetText(capStr.String());
		fTotalUsedValue->SetText(usedStr.String());
		fTotalFreeValue->SetText(freeStr.String());
		fRootVolValue->SetText(GetRootDiskUsage().String());
	}

	fDiskListView->Invalidate();
}

void DiskView::Draw(BRect updateRect)
{
	BView::Draw(updateRect);
}

void DiskView::Hide()
{
	fPerformanceViewVisible = false;
	BView::Hide();
}

void DiskView::Show()
{
	fPerformanceViewVisible = true;
	if (fScanSem >= 0)
		release_sem(fScanSem);
	BView::Show();
}

void DiskView::_SortItems()
{
	DiskListItem::sSortAscending = fSortAscending;
	switch (fSortMode) {
		case SORT_DISK_BY_DEVICE: default: fDiskListView->SortItems(DiskListItem::CompareDevice); break;
		case SORT_DISK_BY_MOUNT: fDiskListView->SortItems(DiskListItem::CompareMount); break;
		case SORT_DISK_BY_FS: fDiskListView->SortItems(DiskListItem::CompareFS); break;
		case SORT_DISK_BY_TOTAL: fDiskListView->SortItems(DiskListItem::CompareTotal); break;
		case SORT_DISK_BY_USED: fDiskListView->SortItems(DiskListItem::CompareUsed); break;
		case SORT_DISK_BY_FREE: fDiskListView->SortItems(DiskListItem::CompareFree); break;
		case SORT_DISK_BY_PERCENT: fDiskListView->SortItems(DiskListItem::CompareUsage); break;
	}
}

void DiskView::_RestoreSelection(dev_t selectedID)
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
}

void DiskView::_ScanVolumes()
{
	{
		BAutolock locker(fLocker);
		if (locker.IsLocked())
			fVolumeCache.clear();
	}

	BVolumeRoster volRoster;
	BVolume volume;
	volRoster.Rewind();

	while (volRoster.GetNextVolume(&volume) == B_OK) {
		if (volume.Capacity() <= 0) continue;

		DiskInfo info;
		if (GetDiskInfo(volume, info) == B_OK) {
			 BAutolock locker(fLocker);
			 if (locker.IsLocked())
				 fVolumeCache[info.deviceID] = info;
		}
	}
}
