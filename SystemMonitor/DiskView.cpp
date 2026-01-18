#include "DiskView.h"
#include "Utils.h"
#include <LayoutBuilder.h>
#include <StringView.h>
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

const uint32 kMsgDiskDataUpdate = 'dskd';

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
    // Trigger update in thread
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
    strncpy(info.fileSystemType, fsInfo.fsh_name, sizeof(info.fileSystemType));

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
    strncpy(info.mountPoint, mountPath.Path(), sizeof(info.mountPoint));

    // Use volume name if available, otherwise device name
    char volumeName[B_FILE_NAME_LENGTH];
    if (volume.GetName(volumeName) == B_OK && strlen(volumeName) > 0) {
        strncpy(info.deviceName, volumeName, sizeof(info.deviceName));
    } else {
        strncpy(info.deviceName, fsInfo.device_name, sizeof(info.deviceName));
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

    // Process message data
    int32 count = 0;
    type_code type;
    message->GetInfo("disk_info", &type, &count);

    for (int32 i = 0; i < count; i++) {
        const DiskInfo* info;
        ssize_t size;
        if (message->FindData("disk_info", B_RAW_TYPE, i, (const void**)&info, &size) == B_OK) {
            DiskInfo currentDiskInfo = *info;
            activeDevices.insert(currentDiskInfo.deviceID);

            uint64 usedSize = currentDiskInfo.totalSize - currentDiskInfo.freeSize;
            double usagePercent = 0.0;
            if (currentDiskInfo.totalSize > 0) {
                usagePercent = (double)usedSize / currentDiskInfo.totalSize * 100.0;
            }
            char percentStr[16];
            snprintf(percentStr, sizeof(percentStr), "%.1f%%", usagePercent);

            BRow* row;
            if (fDeviceRowMap.find(currentDiskInfo.deviceID) == fDeviceRowMap.end()) {
                // New device, create a new row
                row = new BRow();
                row->SetField(new BStringField(currentDiskInfo.deviceName), kDeviceColumn);
                row->SetField(new BStringField(currentDiskInfo.mountPoint), kMountPointColumn);
                row->SetField(new BStringField(currentDiskInfo.fileSystemType), kFSTypeColumn);
                row->SetField(new BStringField(::FormatBytes(currentDiskInfo.totalSize)), kTotalSizeColumn);
                row->SetField(new BStringField(::FormatBytes(usedSize)), kUsedSizeColumn);
                row->SetField(new BStringField(::FormatBytes(currentDiskInfo.freeSize)), kFreeSizeColumn);
                row->SetField(new BStringField(percentStr), kUsagePercentageColumn);
                fDiskListView->AddRow(row);
                fDeviceRowMap[currentDiskInfo.deviceID] = row;
            } else {
                // Existing device, update the row
                row = fDeviceRowMap[currentDiskInfo.deviceID];

                BStringField* devField = static_cast<BStringField*>(row->GetField(kDeviceColumn));
                if (devField && strcmp(devField->String(), currentDiskInfo.deviceName.String()) != 0)
                    devField->SetString(currentDiskInfo.deviceName);

                BStringField* mntField = static_cast<BStringField*>(row->GetField(kMountPointColumn));
                if (mntField && strcmp(mntField->String(), currentDiskInfo.mountPoint.String()) != 0)
                    mntField->SetString(currentDiskInfo.mountPoint);

                BStringField* fsField = static_cast<BStringField*>(row->GetField(kFSTypeColumn));
                if (fsField && strcmp(fsField->String(), currentDiskInfo.fileSystemType.String()) != 0)
                    fsField->SetString(currentDiskInfo.fileSystemType);

                BString totalStr = ::FormatBytes(currentDiskInfo.totalSize);
                BStringField* totalField = static_cast<BStringField*>(row->GetField(kTotalSizeColumn));
                if (totalField) totalField->SetString(totalStr);

                BString usedStr = ::FormatBytes(usedSize);
                BStringField* usedField = static_cast<BStringField*>(row->GetField(kUsedSizeColumn));
                if (usedField) usedField->SetString(usedStr);

                BString freeStr = ::FormatBytes(currentDiskInfo.freeSize);
                BStringField* freeField = static_cast<BStringField*>(row->GetField(kFreeSizeColumn));
                if (freeField) freeField->SetString(freeStr);

                BStringField* pctField = static_cast<BStringField*>(row->GetField(kUsagePercentageColumn));
                if (pctField) pctField->SetString(percentStr);

                fDiskListView->UpdateRow(row);
            }
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

int32 DiskView::UpdateThread(void* data)
{
    DiskView* view = static_cast<DiskView*>(data);

    while (!view->fTerminated) {
        acquire_sem(view->fScanSem);
        if (view->fTerminated) break;

        BMessage updateMsg(kMsgDiskDataUpdate);
        BVolumeRoster volRoster;
        BVolume volume;
        volRoster.Rewind();

        while (volRoster.GetNextVolume(&volume) == B_OK) {
            if (volume.Capacity() <= 0) continue;

            DiskInfo currentDiskInfo;
            if (GetDiskInfo(volume, currentDiskInfo) == B_OK) {
                updateMsg.AddData("disk_info", B_RAW_TYPE, &currentDiskInfo, sizeof(DiskInfo));
            }
        }

        if (view->Window()) {
            view->Window()->PostMessage(&updateMsg, view);
        }
    }
    return B_OK;
}

void DiskView::Draw(BRect updateRect)
{
    BView::Draw(updateRect);
}
