#ifndef DISKVIEW_H
#define DISKVIEW_H

#include <View.h>
#include <Locker.h>
#include <String.h>
#include <Volume.h>
#include <map>
#include <set>
#include <atomic>

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

private:
    static int32 UpdateThread(void* data);
    void UpdateData(BMessage* message);
    status_t GetDiskInfo(BVolume& volume, DiskInfo& info);
    
    BBox* fDiskInfoBox;
    BListView* fDiskListView;
    
    BLocker fLocker;
	std::map<dev_t, DiskListItem*> fDeviceItemMap;
    std::set<DiskListItem*> fVisibleItems;

    thread_id fUpdateThread;
    sem_id fScanSem;
    std::atomic<bool> fTerminated;
};

#endif // DISKVIEW_H
