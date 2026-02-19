#ifndef DISKVIEW_H
#define DISKVIEW_H

#include <View.h>
#include <Locker.h>
#include <String.h>
#include <Volume.h>
#include <map>
#include <set>
#include <atomic>
#include <Font.h>
#include <NodeMonitor.h>

class BBox;
class BListView;
class BListItem;
class DiskListItem; // Forward declaration

struct DiskInfo {
    BString deviceName;
    BString mountPoint;
    BString fileSystemType;
    uint64 totalSize;
    uint64 freeSize;
	dev_t deviceID;
};

const uint32 kMsgDiskDataUpdate = 'dskd';

class DiskView : public BView {
public:
    DiskView();
    virtual ~DiskView();
    
    virtual void AttachedToWindow();
    virtual void DetachedFromWindow();
    virtual void Pulse();
    virtual void MessageReceived(BMessage* message);
    virtual void Draw(BRect updateRect);

    void SetRefreshInterval(bigtime_t interval);
    void SetPerformanceViewVisible(bool visible) { fPerformanceViewVisible = visible; }

    float DeviceWidth() const { return fDeviceWidth; }
    float MountWidth() const { return fMountWidth; }
    float FSWidth() const { return fFSWidth; }
    float TotalWidth() const { return fTotalWidth; }
    float UsedWidth() const { return fUsedWidth; }
    float FreeWidth() const { return fFreeWidth; }
    float PercentWidth() const { return fPercentWidth; }

private:
    static int32 UpdateThread(void* data);
    void UpdateData(BMessage* message);
    status_t GetDiskInfo(BVolume& volume, DiskInfo& info);
    
    BBox* fDiskInfoBox;
    BListView* fDiskListView;
    
    BLocker fLocker; // Protects fVolumeCache and fDeviceItemMap
	std::map<dev_t, DiskListItem*> fDeviceItemMap;
    std::set<DiskListItem*> fVisibleItems;

    BFont fCachedFont;

    thread_id fUpdateThread;
    sem_id fScanSem;
    std::atomic<bool> fTerminated;
    std::atomic<bool> fPerformanceViewVisible;
    std::atomic<bigtime_t> fRefreshInterval;
    int32 fListGeneration;

    float fDeviceWidth;
    float fMountWidth;
    float fFSWidth;
    float fTotalWidth;
    float fUsedWidth;
    float fFreeWidth;
    float fPercentWidth;

    void _ScanVolumes();
    std::map<dev_t, DiskInfo> fVolumeCache;
};

#endif // DISKVIEW_H
