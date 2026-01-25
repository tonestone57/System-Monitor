#include "DiskView.h"
#include "Utils.h"
#include <LayoutBuilder.h>
#include <StringView.h>
#include <OS.h>
#include <cstdio>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>
#include <VolumeRoster.h>
#include <fs_info.h>
#include <ListView.h>
#include <ListItem.h>
#include <Box.h>
#include <Font.h>
#include <set>
#include <Messenger.h>
#include <Catalog.h>
#include <ScrollView.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DiskView"

const float kDiskDeviceWidth = 120;
const float kDiskMountWidth = 120;
const float kDiskFSWidth = 80;
const float kDiskTotalWidth = 100;
const float kDiskUsedWidth = 100;
const float kDiskFreeWidth = 100;
const float kDiskPercentWidth = 80;

class DiskListItem : public BListItem {
public:
    DiskListItem(const BString& device, const BString& mount, const BString& fs,
                 uint64 total, uint64 used, uint64 free, double percent)
        : BListItem(),
          fDevice(device), fMount(mount), fFS(fs),
          fTotal(total), fUsed(used), fFree(free), fPercent(percent)
    {
    }

    void Update(const BString& device, const BString& mount, const BString& fs,
                 uint64 total, uint64 used, uint64 free, double percent) {
        fDevice = device;
        fMount = mount;
        fFS = fs;
        fTotal = total;
        fUsed = used;
        fFree = free;
        fPercent = percent;
    }

    virtual void DrawItem(BView* owner, BRect itemRect, bool complete = false) {
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

        BFont font;
        owner->GetFont(&font);
        font_height fh;
        font.GetHeight(&fh);

        float x = itemRect.left + 5;
        float y = itemRect.bottom - fh.descent;

        auto drawTruncated = [&](const BString& str, float width) {
             BString out = str;
             font.TruncateString(&out, B_TRUNCATE_MIDDLE, width - 10);
             owner->DrawString(out.String(), BPoint(x, y));
             x += width;
        };

        auto drawRight = [&](const BString& str, float width) {
             float w = owner->StringWidth(str.String());
             owner->DrawString(str.String(), BPoint(x + width - w - 5, y));
             x += width;
        };

        drawTruncated(fDevice, kDiskDeviceWidth);
        drawTruncated(fMount, kDiskMountWidth);

        // FS
        BString fsTrunc = fFS;
        font.TruncateString(&fsTrunc, B_TRUNCATE_END, kDiskFSWidth - 10);
        owner->DrawString(fsTrunc.String(), BPoint(x, y));
        x += kDiskFSWidth;

        drawRight(FormatBytes(fTotal), kDiskTotalWidth);
        drawRight(FormatBytes(fUsed), kDiskUsedWidth);
        drawRight(FormatBytes(fFree), kDiskFreeWidth);

        char buf[32];
        snprintf(buf, sizeof(buf), "%.1f%%", fPercent);
        drawRight(buf, kDiskPercentWidth);
    }

private:
    BString fDevice;
    BString fMount;
    BString fFS;
    uint64 fTotal;
    uint64 fUsed;
    uint64 fFree;
    double fPercent;

public:
    static int CompareUsage(const void* first, const void* second) {
        const DiskListItem* item1 = *(const DiskListItem**)first;
        const DiskListItem* item2 = *(const DiskListItem**)second;
        if (item1->fPercent > item2->fPercent) return -1;
        if (item1->fPercent < item2->fPercent) return 1;
        return 0;
    }
};


DiskView::DiskView()
    : BView("DiskView", B_WILL_DRAW | B_PULSE_NEEDED),
      fUpdateThread(-1),
      fScanSem(-1),
      fTerminated(false)
{
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    fScanSem = create_sem(0, "disk scan sem");

    fDiskInfoBox = new BBox("DiskInfoBox");
    fDiskInfoBox->SetLabel(B_TRANSLATE("Disk Volumes"));

    // Header view
    BGroupView* headerView = new BGroupView(B_HORIZONTAL, 0);
    headerView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    
    auto addHeader = [&](const char* label, float width, alignment align = B_ALIGN_LEFT) {
        BStringView* sv = new BStringView(NULL, label);
        sv->SetExplicitMinSize(BSize(width, B_SIZE_UNSET));
        sv->SetExplicitMaxSize(BSize(width, B_SIZE_UNSET));
        sv->SetAlignment(align);
        sv->SetFont(be_bold_font);
        headerView->AddChild(sv);
    };

    addHeader(B_TRANSLATE("Device"), kDiskDeviceWidth);
    addHeader(B_TRANSLATE("Mount Point"), kDiskMountWidth);
    addHeader(B_TRANSLATE("FS Type"), kDiskFSWidth);
    addHeader(B_TRANSLATE("Total"), kDiskTotalWidth, B_ALIGN_RIGHT);
    addHeader(B_TRANSLATE("Used"), kDiskUsedWidth, B_ALIGN_RIGHT);
    addHeader(B_TRANSLATE("Free"), kDiskFreeWidth, B_ALIGN_RIGHT);
    addHeader(B_TRANSLATE("Usage"), kDiskPercentWidth, B_ALIGN_RIGHT);

    headerView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 20));

    fDiskListView = new BListView("disk_list", B_SINGLE_SELECTION_LIST, B_WILL_DRAW | B_NAVIGABLE);
    BScrollView* diskScrollView = new BScrollView("disk_scroll", fDiskListView, 0, false, true, true);

    BStringView* noteView = new BStringView("io_note", B_TRANSLATE("Real-time Disk I/O monitoring is not supported on this system."));
    noteView->SetAlignment(B_ALIGN_CENTER);
    BFont font(be_plain_font);
    font.SetSize(font.Size() * 0.9f);
    noteView->SetFont(&font);
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
    fVisibleItems.clear();
}

void DiskView::AttachedToWindow()
{
    BView::AttachedToWindow();
    fTerminated = false;

    if (fScanSem < 0)
        fScanSem = create_sem(0, "disk scan sem");

    fUpdateThread = spawn_thread(UpdateThread, "DiskView Update", B_NORMAL_PRIORITY, this);
    if (fUpdateThread >= 0)
        resume_thread(fUpdateThread);
}

void DiskView::DetachedFromWindow()
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
    BView::DetachedFromWindow();
}

void DiskView::MessageReceived(BMessage* message)
{
    if (message->what == kMsgDiskDataUpdate) {
        UpdateData(message);
    } else {
        BView::MessageReceived(message);
    }
}

void DiskView::Pulse()
{
    if (!IsHidden() && fScanSem >= 0) release_sem(fScanSem);
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
        status_t err = acquire_sem(view->fScanSem);
        if (err != B_OK) {
            if (view->fTerminated) break;
            if (err == B_INTERRUPTED) continue;
            break;
        }

        int32 count;
        if (get_sem_count(view->fScanSem, &count) == B_OK && count > 0)
            acquire_sem_etc(view->fScanSem, count, B_RELATIVE_TIMEOUT, 0);

        BMessage updateMsg(kMsgDiskDataUpdate);
        BVolumeRoster volRoster;
        BVolume volume;
        volRoster.Rewind();

        while (volRoster.GetNextVolume(&volume) == B_OK) {
             if (volume.Capacity() <= 0) continue;

             DiskInfo currentDiskInfo;
             if (view->GetDiskInfo(volume, currentDiskInfo) != B_OK) {
                 continue;
             }
             if (currentDiskInfo.totalSize == 0) continue;

             BMessage volMsg;
             volMsg.AddInt32("device_id", currentDiskInfo.deviceID);
             volMsg.AddString("device_name", currentDiskInfo.deviceName);
             volMsg.AddString("mount_point", currentDiskInfo.mountPoint);
             volMsg.AddString("fs_type", currentDiskInfo.fileSystemType);
             volMsg.AddUInt64("total_size", currentDiskInfo.totalSize);
             volMsg.AddUInt64("free_size", currentDiskInfo.freeSize);

             updateMsg.AddMessage("volume", &volMsg);
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

    std::set<dev_t> activeDevices;
    int32 count = 0;
    type_code type;
    message->GetInfo("volume", &type, &count);

    bool listChanged = false;

    for (int32 i = 0; i < count; i++) {
        BMessage volMsg;
        if (message->FindMessage("volume", i, &volMsg) != B_OK) continue;

        dev_t deviceID;
        if (volMsg.FindInt32("device_id", (int32*)&deviceID) != B_OK) continue;

        activeDevices.insert(deviceID);

        BString deviceName = volMsg.FindString("device_name");
        BString mountPoint = volMsg.FindString("mount_point");
        BString fsType = volMsg.FindString("fs_type");
        uint64 totalSize = 0, freeSize = 0;
        volMsg.FindUInt64("total_size", &totalSize);
        volMsg.FindUInt64("free_size", &freeSize);

        uint64 usedSize = totalSize - freeSize;
        double usagePercent = 0.0;
        if (totalSize > 0) {
            usagePercent = (double)usedSize / totalSize * 100.0;
        }

		DiskListItem* item;
		if (fDeviceItemMap.find(deviceID) == fDeviceItemMap.end()) {
			item = new DiskListItem(deviceName, mountPoint, fsType, totalSize, usedSize, freeSize, usagePercent);
			fDiskListView->AddItem(item);
			fDeviceItemMap[deviceID] = item;
            fVisibleItems.insert(item);
            listChanged = true;
		} else {
			item = fDeviceItemMap[deviceID];
            // Ideally check for changes before calling Invalidate
            item->Update(deviceName, mountPoint, fsType, totalSize, usedSize, freeSize, usagePercent);
		}
    }

	for (auto it = fDeviceItemMap.begin(); it != fDeviceItemMap.end();) {
		if (activeDevices.find(it->first) == activeDevices.end()) {
			DiskListItem* item = it->second;
            if (fVisibleItems.find(item) != fVisibleItems.end()) {
			    fDiskListView->RemoveItem(item);
                fVisibleItems.erase(item);
            }
			delete item;
			it = fDeviceItemMap.erase(it);
            listChanged = true;
		} else {
			++it;
		}
	}
    fDiskListView->SortItems(DiskListItem::CompareUsage);
    fDiskListView->Invalidate();

    fLocker.Unlock();
}

void DiskView::Draw(BRect updateRect)
{
    BView::Draw(updateRect);
}
