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

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DiskView"

const float kBaseDiskDeviceWidth = 120;
const float kBaseDiskMountWidth = 120;
const float kBaseDiskFSWidth = 80;
const float kBaseDiskTotalWidth = 100;
const float kBaseDiskUsedWidth = 100;
const float kBaseDiskFreeWidth = 100;
const float kBaseDiskPercentWidth = 80;

class DiskListItem : public BListItem {
public:
	DiskListItem(dev_t deviceID, const BString& device, const BString& mount, const BString& fs,
				 uint64 total, uint64 used, uint64 free, double percent, const BFont* font, DiskView* view)
		: BListItem(), fDeviceID(deviceID), fGeneration(0), fView(view)
	{
		Update(device, mount, fs, total, used, free, percent, font, true);
	}

	void SetGeneration(int32 generation) { fGeneration = generation; }
	int32 Generation() const { return fGeneration; }
	dev_t DeviceID() const { return fDeviceID; }

	void Update(const BString& device, const BString& mount, const BString& fs,
				 uint64 total, uint64 used, uint64 free, double percent, const BFont* font, bool force = false) {
		bool deviceChanged = force || fDevice != device;
		bool mountChanged = force || fMount != mount;
		bool fsChanged = force || fFS != fs;
		bool totalChanged = force || fTotal != total;
		bool usedChanged = force || fUsed != used;
		bool freeChanged = force || fFree != free;
		bool percentChanged = force || fPercent != percent;

		fDevice = device;
		fMount = mount;
		fFS = fs;
		fTotal = total;
		fUsed = used;
		fFree = free;
		fPercent = percent;

		if (totalChanged)
			FormatBytes(fCachedTotal, fTotal);
		if (usedChanged)
			FormatBytes(fCachedUsed, fUsed);
		if (freeChanged)
			FormatBytes(fCachedFree, fFree);
		if (percentChanged)
			fCachedPercent.SetToFormat("%.1f%%", fPercent);

		if (deviceChanged) {
			if (font && fView) {
				fTruncatedDevice = fDevice;
				font->TruncateString(&fTruncatedDevice, B_TRUNCATE_MIDDLE, fView->DeviceWidth() - 10);
			} else
				fTruncatedDevice = fDevice;
		}

		if (mountChanged) {
			if (font && fView) {
				fTruncatedMount = fMount;
				font->TruncateString(&fTruncatedMount, B_TRUNCATE_MIDDLE, fView->MountWidth() - 10);
			} else
				fTruncatedMount = fMount;
		}

		if (fsChanged) {
			if (font && fView) {
				fTruncatedFS = fFS;
				font->TruncateString(&fTruncatedFS, B_TRUNCATE_END, fView->FSWidth() - 10);
			} else
				fTruncatedFS = fFS;
		}
	}

	virtual void DrawItem(BView* owner, BRect itemRect, bool complete = false) {
		if (!fView) return;
		if (IsSelected() || complete) {
			rgb_color color;
			if (IsSelected()) color = ui_color(B_LIST_SELECTED_BACKGROUND_COLOR);
			else color = ui_color(B_LIST_BACKGROUND_COLOR);
			owner->SetHighColor(color);
			owner->FillRect(itemRect);
		}

		rgb_color textColor;
		if (IsSelected()) textColor = ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR);
		else textColor = ui_color(B_LIST_ITEM_TEXT_COLOR);
		owner->SetHighColor(textColor);

		font_height fh;
		owner->GetFontHeight(&fh);
		float x = itemRect.left + 5;
		float y = itemRect.bottom - fh.descent;

		auto drawRight = [&](const BString& str, float width) {
			 float w = owner->StringWidth(str.String());
			 owner->DrawString(str.String(), BPoint(x + width - w - 5, y));
			 x += width;
		};

		owner->DrawString(fTruncatedDevice.String(), BPoint(x, y));
		x += fView->DeviceWidth();

		owner->DrawString(fTruncatedMount.String(), BPoint(x, y));
		x += fView->MountWidth();

		owner->DrawString(fTruncatedFS.String(), BPoint(x, y));
		x += fView->FSWidth();

		drawRight(fCachedTotal, fView->TotalWidth());
		drawRight(fCachedUsed, fView->UsedWidth());
		drawRight(fCachedFree, fView->FreeWidth());
		drawRight(fCachedPercent, fView->PercentWidth());
	}

	static int CompareDevice(const void* first, const void* second) {
		const DiskListItem* item1 = *static_cast<const DiskListItem* const*>(first);
		const DiskListItem* item2 = *static_cast<const DiskListItem* const*>(second);
		return strcasecmp(item1->fDevice.String(), item2->fDevice.String());
	}

	static int CompareMount(const void* first, const void* second) {
		const DiskListItem* item1 = *static_cast<const DiskListItem* const*>(first);
		const DiskListItem* item2 = *static_cast<const DiskListItem* const*>(second);
		return strcasecmp(item1->fMount.String(), item2->fMount.String());
	}

	static int CompareFS(const void* first, const void* second) {
		const DiskListItem* item1 = *static_cast<const DiskListItem* const*>(first);
		const DiskListItem* item2 = *static_cast<const DiskListItem* const*>(second);
		return strcasecmp(item1->fFS.String(), item2->fFS.String());
	}

	static int CompareTotal(const void* first, const void* second) {
		const DiskListItem* item1 = *static_cast<const DiskListItem* const*>(first);
		const DiskListItem* item2 = *static_cast<const DiskListItem* const*>(second);
		if (item1->fTotal > item2->fTotal) return -1;
		if (item1->fTotal < item2->fTotal) return 1;
		return 0;
	}

	static int CompareUsed(const void* first, const void* second) {
		const DiskListItem* item1 = *static_cast<const DiskListItem* const*>(first);
		const DiskListItem* item2 = *static_cast<const DiskListItem* const*>(second);
		if (item1->fUsed > item2->fUsed) return -1;
		if (item1->fUsed < item2->fUsed) return 1;
		return 0;
	}

	static int CompareFree(const void* first, const void* second) {
		const DiskListItem* item1 = *static_cast<const DiskListItem* const*>(first);
		const DiskListItem* item2 = *static_cast<const DiskListItem* const*>(second);
		if (item1->fFree > item2->fFree) return -1;
		if (item1->fFree < item2->fFree) return 1;
		return 0;
	}

	static int CompareUsage(const void* first, const void* second) {
		const DiskListItem* item1 = *static_cast<const DiskListItem* const*>(first);
		const DiskListItem* item2 = *static_cast<const DiskListItem* const*>(second);
		if (item1->fPercent > item2->fPercent) return -1;
		if (item1->fPercent < item2->fPercent) return 1;
		return 0;
	}

private:
	BString fDevice;
	BString fMount;
	BString fFS;
	uint64 fTotal;
	uint64 fUsed;
	uint64 fFree;
	double fPercent;

	BString fCachedTotal;
	BString fCachedUsed;
	BString fCachedFree;
	BString fCachedPercent;

	BString fTruncatedDevice;
	BString fTruncatedMount;
	BString fTruncatedFS;
	int32 fGeneration;
	dev_t fDeviceID;
	DiskView* fView;
};


DiskView::DiskView()
	: BView("DiskView", B_WILL_DRAW),
	  fUpdateThread(-1),
	  fScanSem(-1),
	  fTerminated(false),
	  fPerformanceViewVisible(true),
	  fRefreshInterval(1000000),
	  fSortMode(SORT_DISK_BY_PERCENT),
	  fListGeneration(0)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fScanSem = create_sem(0, "disk scan sem");

	fDiskInfoBox = new BBox("DiskInfoBox");
	fDiskInfoBox->SetLabel(B_TRANSLATE("Disk Volumes"));

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
	headerView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

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
	addHeader(B_TRANSLATE("Usage"), fPercentWidth, SORT_DISK_BY_PERCENT, B_ALIGN_RIGHT);

	headerView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 20 * scale));

	fDiskListView = new BListView("disk_list", B_SINGLE_SELECTION_LIST, B_WILL_DRAW | B_NAVIGABLE);
	BScrollView* diskScrollView = new BScrollView("disk_scroll", fDiskListView, 0, false, true, true);

	BStringView* noteView = new BStringView("io_note", B_TRANSLATE("Real-time Disk I/O monitoring is not supported on this system."));
	noteView->SetAlignment(B_ALIGN_CENTER);
	BFont noteFont(be_plain_font);
	noteFont.SetSize(noteFont.Size() * 0.9f);
	noteView->SetFont(&noteFont);
	noteView->SetHighColor(ui_color(B_CONTROL_TEXT_COLOR));

	BLayoutBuilder::Group<>(fDiskInfoBox, B_VERTICAL, 0)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING + 15, // Approx font height
				   B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.Add(headerView)
		.Add(diskScrollView)
		.AddStrut(B_USE_DEFAULT_SPACING)
		.Add(noteView);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(0)
		.Add(fDiskInfoBox)
	.End();
}

DiskView::~DiskView()
{
	fTerminated = true;
	if (fScanSem >= 0) delete_sem(fScanSem);
	if (fUpdateThread >= 0) {
		status_t dummy;
		wait_for_thread(fUpdateThread, &dummy);
	}

	// fDiskListView owns the items? No, BListView doesn't own items by default unless we iterate.
	// However, if we empty it, we lose the pointers to items that are also in the map.
	// Correct approach: Empty list without deletion, then delete from map/visible set.
	// Or just clear map since they are same pointers.
	// But we need to delete the objects.
	fDiskListView->MakeEmpty();

	for (auto& pair : fDeviceItemMap) {
		delete pair.second;
	}
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
	stop_watching(BMessenger(this));
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
			fSortMode = (DiskSortMode)mode;
			_SortItems();
			fDiskListView->Invalidate();
		}
	} else if (message->what == B_NODE_MONITOR) {
		int32 opcode;
		if (message->FindInt32("opcode", &opcode) == B_OK) {
			if (opcode == B_DEVICE_MOUNTED) {
				dev_t device;
				if (message->FindInt32("new_device", &device) == B_OK) {
					BVolume volume(device);
					if (volume.InitCheck() == B_OK && volume.Capacity() > 0) {
						DiskInfo info;
						if (GetDiskInfo(volume, info) == B_OK) {
							fLocker.Lock();
							fVolumeCache[info.deviceID] = info;
							fLocker.Unlock();
						}
					}
				}
			} else if (opcode == B_DEVICE_UNMOUNTED) {
				dev_t device;
				if (message->FindInt32("device", &device) == B_OK) {
					fLocker.Lock();
					fVolumeCache.erase(device);
					fLocker.Unlock();
				}
			}
		}
	} else {
		BView::MessageReceived(message);
	}
}

void DiskView::SetRefreshInterval(bigtime_t interval)
{
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
	info.totalSize = fsInfo.total_blocks * fsInfo.block_size;
	info.freeSize = fsInfo.free_blocks * fsInfo.block_size;
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
	info.mountPoint = mountPath.Path();

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

		std::vector<DiskInfo> volumesToPoll;
		if (view->fLocker.Lock()) {
			for (auto const& pair : view->fVolumeCache) {
				 volumesToPoll.push_back(pair.second);
			}
			view->fLocker.Unlock();
		}

		for (auto& info : volumesToPoll) {
			 fs_info fsInfo;
			 if (fs_stat_dev(info.deviceID, &fsInfo) != B_OK) {
				 info.totalSize = 0; // Mark as invalid
				 continue;
			 }

			 // Update dynamic info
			 info.totalSize = static_cast<uint64>(fsInfo.total_blocks) * fsInfo.block_size;
			 info.freeSize = static_cast<uint64>(fsInfo.free_blocks) * fsInfo.block_size;

			 // Update name dynamically
			 if (strlen(fsInfo.volume_name) > 0) {
				 info.deviceName = fsInfo.volume_name;
			 } else {
				 info.deviceName = fsInfo.device_name;
			 }
		}

		if (view->fLocker.Lock()) {
			for (const auto& info : volumesToPoll) {
				 if (info.totalSize == 0) continue;

				 // Update cache
				 if (view->fVolumeCache.count(info.deviceID)) {
					 view->fVolumeCache[info.deviceID] = info;
				 }

				 BMessage volMsg;
				 volMsg.AddInt32("device_id", info.deviceID);
				 volMsg.AddString("device_name", info.deviceName);
				 volMsg.AddString("mount_point", info.mountPoint);
				 volMsg.AddString("fs_type", info.fileSystemType);
				 volMsg.AddUInt64("total_size", info.totalSize);
				 volMsg.AddUInt64("free_size", info.freeSize);

				 updateMsg.AddMessage("volume", &volMsg);
			}
			view->fLocker.Unlock();
		}

		target.SendMessage(&updateMsg);
	}
	return B_OK;
}

void DiskView::UpdateData(BMessage* message)
{
	fLocker.Lock();

	if (!fDiskListView) {
		fLocker.Unlock();
		return;
	}

	// Preserve selection
	int32 selection = fDiskListView->CurrentSelection();
	dev_t selectedID = -1;
	if (selection >= 0) {
		DiskListItem* item = dynamic_cast<DiskListItem*>(fDiskListView->ItemAt(selection));
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

		uint64 usedSize = totalSize - freeSize;
		double usagePercent = 0.0;
		if (totalSize > 0) {
			usagePercent = static_cast<double>(usedSize) / totalSize * 100.0;
		}

		DiskListItem* item;
		auto result = fDeviceItemMap.emplace(deviceID, nullptr);
		if (result.second) {
			item = new DiskListItem(deviceID, deviceName, mountPoint, fsType, totalSize, usedSize, freeSize, usagePercent, &font, this);
			fDiskListView->AddItem(item);
			result.first->second = item;
		} else {
			item = result.first->second;
			item->Update(deviceName, mountPoint, fsType, totalSize, usedSize, freeSize, usagePercent, &font, fontChanged);
		}
		item->SetGeneration(fListGeneration);
	}

	for (auto it = fDeviceItemMap.begin(); it != fDeviceItemMap.end();) {
		if (it->second->Generation() != fListGeneration) {
			DiskListItem* item = it->second;
			fDiskListView->RemoveItem(item);
			delete item;
			it = fDeviceItemMap.erase(it);
		} else {
			++it;
		}
	}
	_SortItems();

	_RestoreSelection(selectedID);

	fDiskListView->Invalidate();

	fLocker.Unlock();
}

void DiskView::Draw(BRect updateRect)
{
	BView::Draw(updateRect);
}

void DiskView::_SortItems()
{
	switch (fSortMode) {
		case SORT_DISK_BY_DEVICE: fDiskListView->SortItems(DiskListItem::CompareDevice); break;
		case SORT_DISK_BY_MOUNT: fDiskListView->SortItems(DiskListItem::CompareMount); break;
		case SORT_DISK_BY_FS: fDiskListView->SortItems(DiskListItem::CompareFS); break;
		case SORT_DISK_BY_TOTAL: fDiskListView->SortItems(DiskListItem::CompareTotal); break;
		case SORT_DISK_BY_USED: fDiskListView->SortItems(DiskListItem::CompareUsed); break;
		case SORT_DISK_BY_FREE: fDiskListView->SortItems(DiskListItem::CompareFree); break;
		case SORT_DISK_BY_PERCENT: default: fDiskListView->SortItems(DiskListItem::CompareUsage); break;
	}
}

void DiskView::_RestoreSelection(dev_t selectedID)
{
	if (selectedID == -1)
		return;

	for (int32 i = 0; i < fDiskListView->CountItems(); i++) {
		DiskListItem* item = dynamic_cast<DiskListItem*>(fDiskListView->ItemAt(i));
		if (item && item->DeviceID() == selectedID) {
			fDiskListView->Select(i);
			break;
		}
	}
}

void DiskView::_ScanVolumes()
{
	fLocker.Lock();
	fVolumeCache.clear();
	fLocker.Unlock();

	BVolumeRoster volRoster;
	BVolume volume;
	volRoster.Rewind();

	while (volRoster.GetNextVolume(&volume) == B_OK) {
		if (volume.Capacity() <= 0) continue;

		DiskInfo info;
		if (GetDiskInfo(volume, info) == B_OK) {
			 fLocker.Lock();
			 fVolumeCache[info.deviceID] = info;
			 fLocker.Unlock();
		}
	}
}
