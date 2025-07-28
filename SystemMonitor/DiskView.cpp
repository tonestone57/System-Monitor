#include "DiskView.h"
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

DiskView::DiskView(BRect frame)
    : BView(frame, "DiskView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_PULSE_NEEDED)
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
    // Child views are deleted automatically
}

void DiskView::AttachedToWindow()
{
    UpdateData();
    BView::AttachedToWindow();
}

void DiskView::Pulse()
{
    UpdateData();
}

BString DiskView::FormatBytes(uint64 bytes) {
    BString str;
    double kb = bytes / 1024.0;
    double mb = kb / 1024.0;
    double gb = mb / 1024.0;

    if (gb >= 1.0) {
        str.SetToFormat("%.2f GiB", gb);
    } else if (mb >= 1.0) {
        str.SetToFormat("%.2f MiB", mb);
    } else {
        str.SetToFormat("%.0f KiB", kb);
    }
    return str;
}

status_t DiskView::GetDiskInfo(BVolume& volume, DiskInfo& info) {
    fs_info fsInfo;
    status_t status = fs_stat_dev(volume.Device(), &fsInfo);
    if (status != B_OK) {
        return status;
    }

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

    // Clear existing rows
    while (BRow* row = fDiskListView->RowAt(0)) {
        fDiskListView->RemoveRow(row);
        delete row;
    }

    BVolumeRoster volRoster;
    BVolume volume;
    volRoster.Rewind();

    while (volRoster.GetNextVolume(&volume) == B_OK) {
        if (volume.Capacity() <= 0) continue;

        DiskInfo currentDiskInfo;
        if (GetDiskInfo(volume, currentDiskInfo) != B_OK) {
            continue;
        }

        if (currentDiskInfo.totalSize == 0) continue;

        BRow* row = new BRow();
        struct RowGuard {
            BRow*& fRow;
            RowGuard(BRow*& row) : fRow(row) {}
            ~RowGuard() { if (fRow) delete fRow; }
        } rowGuard(row);

        uint64 usedSize = currentDiskInfo.totalSize - currentDiskInfo.freeSize;
        double usagePercent = 0.0;
        if (currentDiskInfo.totalSize > 0) {
            usagePercent = (double)usedSize / currentDiskInfo.totalSize * 100.0;
        }
        char percentStr[16];
        snprintf(percentStr, sizeof(percentStr), "%.1f%%", usagePercent);

        row->SetField(new BStringField(currentDiskInfo.deviceName), kDeviceColumn);
        row->SetField(new BStringField(currentDiskInfo.mountPoint), kMountPointColumn);
        row->SetField(new BStringField(currentDiskInfo.fileSystemType), kFSTypeColumn);
        row->SetField(new BStringField(FormatBytes(currentDiskInfo.totalSize)), kTotalSizeColumn);
        row->SetField(new BStringField(FormatBytes(usedSize)), kUsedSizeColumn);
        row->SetField(new BStringField(FormatBytes(currentDiskInfo.freeSize)), kFreeSizeColumn);
        row->SetField(new BStringField(percentStr), kUsagePercentageColumn);

        fDiskListView->AddRow(row);
        row = nullptr; // The BColumnListView now owns the row
    }

    fLocker.Unlock();
}

void DiskView::Draw(BRect updateRect)
{
    BView::Draw(updateRect);
}
