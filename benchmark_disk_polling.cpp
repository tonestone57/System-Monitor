#include <Application.h>
#include <VolumeRoster.h>
#include <Volume.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>
#include <fs_info.h>
#include <String.h>
#include <stdio.h>
#include <OS.h>

struct DiskInfo {
    BString deviceName;
    BString mountPoint;
    BString fileSystemType;
    uint64 totalSize;
    uint64 freeSize;
    dev_t deviceID;
};

status_t GetDiskInfo(BVolume& volume, DiskInfo& info) {
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

int main() {
    // We need a BApplication for some services, though maybe not strictly for BVolumeRoster?
    // BVolumeRoster works without BApp usually.
    // But let's be safe.
    BApplication app("application/x-vnd.bench-disk");

    printf("Benchmarking Disk Polling...\n");

    const int iterations = 100;
    bigtime_t start = system_time();

    for (int i = 0; i < iterations; i++) {
        BVolumeRoster volRoster;
        BVolume volume;
        volRoster.Rewind();
        while (volRoster.GetNextVolume(&volume) == B_OK) {
            if (volume.Capacity() <= 0) continue;
            DiskInfo info;
            GetDiskInfo(volume, info);
        }
    }

    bigtime_t end = system_time();
    printf("Full Polling (Current): %lld us per iteration\n", (end - start) / iterations);

    // Simulate Cached approach
    // First, populate cache
    struct CachedInfo {
        dev_t dev;
        BString deviceName;
        BString mountPoint;
        BString fileSystemType;
        uint64 totalSize;
    };

    // Simple array for cache simulation
    CachedInfo cache[64];
    int count = 0;

    BVolumeRoster volRoster;
    BVolume volume;
    volRoster.Rewind();
    while (volRoster.GetNextVolume(&volume) == B_OK) {
        if (volume.Capacity() <= 0) continue;
        DiskInfo info;
        GetDiskInfo(volume, info);
        cache[count].dev = info.deviceID;
        cache[count].deviceName = info.deviceName;
        cache[count].mountPoint = info.mountPoint;
        cache[count].fileSystemType = info.fileSystemType;
        cache[count].totalSize = info.totalSize;
        count++;
    }

    start = system_time();

    for (int i = 0; i < iterations; i++) {
        for (int j = 0; j < count; j++) {
            fs_info fsInfo;
            if (fs_stat_dev(cache[j].dev, &fsInfo) == B_OK) {
                // Update dynamic part
                uint64 freeSize = fsInfo.free_blocks * fsInfo.block_size;
                // In real app we would construct message here
            }
        }
    }

    end = system_time();
    printf("Cached Polling (Optimized): %lld us per iteration\n", (end - start) / iterations);

    return 0;
}
