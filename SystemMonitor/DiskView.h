#ifndef DISKVIEW_H
#define DISKVIEW_H

#include <View.h>
#include <Locker.h>
#include <String.h>
#include <Volume.h>
#include <unordered_map>
#include <vector>
#include <set>
#include <atomic>
#include <Font.h>
#include <NodeMonitor.h>

class BBox;
class BListView;
class BListItem;
class ClickableHeaderView;
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

enum DiskSortMode {
	SORT_DISK_BY_DEVICE,
	SORT_DISK_BY_MOUNT,
	SORT_DISK_BY_FS,
	SORT_DISK_BY_TOTAL,
	SORT_DISK_BY_USED,
	SORT_DISK_BY_FREE,
	SORT_DISK_BY_PERCENT
};

class DiskView : public BView {
public:
	DiskView();
	virtual ~DiskView();

	virtual void AttachedToWindow();
	virtual void DetachedFromWindow();
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
	std::vector<ClickableHeaderView*> fHeaders;

	BLocker fLocker; // Protects fVolumeCache and fDeviceItemMap
	std::unordered_map<dev_t, DiskListItem*> fDeviceItemMap;

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

	DiskSortMode fSortMode;

	void _SortItems();
	void _RestoreSelection(dev_t selectedID);
	void _ScanVolumes();
	std::unordered_map<dev_t, DiskInfo> fVolumeCache;
};

#endif // DISKVIEW_H
