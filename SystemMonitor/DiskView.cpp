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
    : BView("DiskView", B_WILL_DRAW | B_PULSE_NEEDED)
{
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

    fDiskInfoBox = new BBox("DiskInfoBox");
    fDiskInfoBox->SetLabel("Disk Volumes");

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

    fDiskListView->AddColumn(new BStringColumn("Device", 120, 50, 300, B_TRUNCATE_MIDDLE), kDeviceColumn);
    fDiskListView->AddColumn(new BStringColumn("Mount Point", 120, 50, 300, B_TRUNCATE_MIDDLE), kMountPointColumn);
    fDiskListView->AddColumn(new BStringColumn("FS Type", 80, 40, 150, B_TRUNCATE_END), kFSTypeColumn);
    fDiskListView->AddColumn(new BStringColumn("Total Size", 100, 50, 150, B_TRUNCATE_END, B_ALIGN_RIGHT), kTotalSizeColumn);
    fDiskListView->AddColumn(new BStringColumn("Used Size", 100, 50, 150, B_TRUNCATE_END, B_ALIGN_RIGHT), kUsedSizeColumn);
    fDiskListView->AddColumn(new BStringColumn("Free Size", 100, 50, 150, B_TRUNCATE_END, B_ALIGN_RIGHT), kFreeSizeColumn);
    fDiskListView->AddColumn(new BStringColumn("Usage %", 80, 40, 100, B_TRUNCATE_END, B_ALIGN_RIGHT), kUsagePercentageColumn);

    fDiskListView->SetSortColumn(fDiskListView->ColumnAt(kMountPointColumn), true, true);

    // Use layout to properly position the ColumnListView
    BLayoutBuilder::Group<>(fDiskInfoBox, B_VERTICAL, 0)
        .SetInsets(B_USE_DEFAULT_SPACING, fh.ascent + fh.descent + fh.leading + B_USE_DEFAULT_SPACING, 
                   B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
        .Add(fDiskListView);

    // Main layout for the DiskView
    BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
        .SetInsets(0)
        .Add(fDiskInfoBox)
    .End();
}

DiskView::~DiskView()
{
	for (auto const& [dev, row] : fDeviceRowMap)
		delete row;
}

void DiskView::AttachedToWindow()
{
    BView::AttachedToWindow();
    UpdateData();
}

void DiskView::Pulse()
{
    UpdateData();
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

void DiskView::UpdateData()
{
    fLocker.Lock();

    if (!fDiskListView) {
        fLocker.Unlock();
        return;
    }

	std::set<dev_t> activeDevices;
    BVolumeRoster volRoster;
    BVolume volume;
    volRoster.Rewind();

    while (volRoster.GetNextVolume(&volume) == B_OK) {
        if (volume.Capacity() <= 0) continue;

        DiskInfo currentDiskInfo;
        if (GetDiskInfo(volume, currentDiskInfo) != B_OK) {
            continue;
        }
		activeDevices.insert(currentDiskInfo.deviceID);

        if (currentDiskInfo.totalSize == 0) continue;

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
			row->SetField(new BStringField(currentDiskInfo.deviceName), kDeviceColumn);
			row->SetField(new BStringField(currentDiskInfo.mountPoint), kMountPointColumn);
			row->SetField(new BStringField(currentDiskInfo.fileSystemType), kFSTypeColumn);
			row->SetField(new BStringField(::FormatBytes(currentDiskInfo.totalSize)), kTotalSizeColumn);
			row->SetField(new BStringField(::FormatBytes(usedSize)), kUsedSizeColumn);
			row->SetField(new BStringField(::FormatBytes(currentDiskInfo.freeSize)), kFreeSizeColumn);
			row->SetField(new BStringField(percentStr), kUsagePercentageColumn);
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
