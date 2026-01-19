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
#include <private/interface/ColumnListView.h>
#include <private/interface/ColumnTypes.h>
#include <Box.h>
#include <Font.h>
#include <set>
#include <Messenger.h>
#include <Catalog.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DiskView"

// Define column constants for BColumnListView
enum {
    kDeviceColumn,
    kMountPointColumn,
    kFSTypeColumn,
    kTotalSizeColumn,
    kFreeSizeColumn,
    kUsedSizeColumn,
    kUsagePercentageColumn
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

    // Calculate proper positioning for ColumnListView inside BBox
    BRect clvRect = fDiskInfoBox->Bounds();
    clvRect.InsetBy(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
    
    // Adjust for label height
    font_height fh;
    fDiskInfoBox->GetFontHeight(&fh);
    clvRect.top += fh.ascent + fh.descent + fh.leading + B_USE_DEFAULT_SPACING;

    fDiskListView = new BColumnListView(clvRect, "disk_clv",
                                        B_FOLLOW_ALL_SIDES,
                                        B_WILL_DRAW | B_NAVIGABLE,
                                        B_PLAIN_BORDER, true);

    fDiskListView->AddColumn(new BStringColumn(B_TRANSLATE("Device"), 120, 50, 300, B_TRUNCATE_MIDDLE), kDeviceColumn);
    fDiskListView->AddColumn(new BStringColumn(B_TRANSLATE("Mount Point"), 120, 50, 300, B_TRUNCATE_MIDDLE), kMountPointColumn);
    fDiskListView->AddColumn(new BStringColumn(B_TRANSLATE("FS Type"), 80, 40, 150, B_TRUNCATE_END), kFSTypeColumn);
    fDiskListView->AddColumn(new BStringColumn(B_TRANSLATE("Total Size"), 100, 50, 150, B_TRUNCATE_END, B_ALIGN_RIGHT), kTotalSizeColumn);
    fDiskListView->AddColumn(new BStringColumn(B_TRANSLATE("Used Size"), 100, 50, 150, B_TRUNCATE_END, B_ALIGN_RIGHT), kUsedSizeColumn);
    fDiskListView->AddColumn(new BStringColumn(B_TRANSLATE("Free Size"), 100, 50, 150, B_TRUNCATE_END, B_ALIGN_RIGHT), kFreeSizeColumn);
    fDiskListView->AddColumn(new BStringColumn(B_TRANSLATE("Usage %"), 80, 40, 100, B_TRUNCATE_END, B_ALIGN_RIGHT), kUsagePercentageColumn);

    fDiskListView->SetSortColumn(fDiskListView->ColumnAt(kMountPointColumn), true, true);

    // Add explanatory note
    BStringView* noteView = new BStringView("io_note", B_TRANSLATE("Real-time Disk I/O monitoring is not supported on this system."));
    noteView->SetAlignment(B_ALIGN_CENTER);
    BFont font(be_plain_font);
    font.SetSize(font.Size() * 0.9f); // Slightly smaller
    noteView->SetFont(&font);
    noteView->SetHighColor(ui_color(B_CONTROL_TEXT_COLOR)); // Ensure visibility

    // Use layout to properly position the ColumnListView
    BLayoutBuilder::Group<>(fDiskInfoBox, B_VERTICAL, 0)
        .SetInsets(B_USE_DEFAULT_SPACING, fh.ascent + fh.descent + fh.leading + B_USE_DEFAULT_SPACING, 
                   B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
        .Add(fDiskListView)
        .AddStrut(B_USE_DEFAULT_SPACING)
        .Add(noteView);

    // Main layout for the DiskView
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
    if (fScanSem >= 0) release_sem(fScanSem);
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

    // Get mount point
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

    // Use volume name if available, otherwise device name
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
            // If the semaphore is bad (e.g. deleted), we must exit
            break;
        }

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
        char percentStr[16];
        snprintf(percentStr, sizeof(percentStr), "%.1f%%", usagePercent);

		BRow* row;
		if (fDeviceRowMap.find(deviceID) == fDeviceRowMap.end()) {
			// New device, create a new row
			row = new BRow();
			row->SetField(new BStringField(deviceName), kDeviceColumn);
			row->SetField(new BStringField(mountPoint), kMountPointColumn);
			row->SetField(new BStringField(fsType), kFSTypeColumn);
			row->SetField(new BStringField(::FormatBytes(totalSize)), kTotalSizeColumn);
			row->SetField(new BStringField(::FormatBytes(usedSize)), kUsedSizeColumn);
			row->SetField(new BStringField(::FormatBytes(freeSize)), kFreeSizeColumn);
			row->SetField(new BStringField(percentStr), kUsagePercentageColumn);
			fDiskListView->AddRow(row);
			fDeviceRowMap[deviceID] = row;
		} else {
			// Existing device, update the row
			row = fDeviceRowMap[deviceID];
            bool changed = false;
            // Only update fields if changed (optimization)
            auto updateField = [&](int index, const char* newVal) {
                BStringField* f = static_cast<BStringField*>(row->GetField(index));
                if (f) {
                    if (strcmp(f->String(), newVal) != 0) {
                        f->SetString(newVal);
                        changed = true;
                    }
                } else {
                    row->SetField(new BStringField(newVal), index);
                    changed = true;
                }
            };

            updateField(kDeviceColumn, deviceName);
            updateField(kMountPointColumn, mountPoint);
            updateField(kFSTypeColumn, fsType);
            updateField(kTotalSizeColumn, ::FormatBytes(totalSize));
            updateField(kUsedSizeColumn, ::FormatBytes(usedSize));
            updateField(kFreeSizeColumn, ::FormatBytes(freeSize));
            updateField(kUsagePercentageColumn, percentStr);

            if (changed)
			    fDiskListView->UpdateRow(row);
		}
    }

	// Remove devices that are no longer present
	for (auto it = fDeviceRowMap.begin(); it != fDeviceRowMap.end();) {
		if (activeDevices.find(it->first) == activeDevices.end()) {
			BRow* row = it->second;
			fDiskListView->RemoveRow(row);
			delete row;
			it = fDeviceRowMap.erase(it);
		} else {
			++it;
		}
	}

    fLocker.Unlock();
}

void DiskView::Draw(BRect updateRect)
{
    BView::Draw(updateRect);
}
