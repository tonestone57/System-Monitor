#ifndef DISKVIEW_H
#define DISKVIEW_H

#include <View.h>
#include <Locker.h>
#include <String.h>
#include <Volume.h>
#include <map>

class BBox;
class BColumnListView;
class BRow;

struct DiskInfo {
    char deviceName[B_FILE_NAME_LENGTH];
    char mountPoint[B_PATH_NAME_LENGTH];
    char fileSystemType[B_OS_NAME_LENGTH];
    uint64 totalSize;
    uint64 freeSize;
	dev_t deviceID;
};

class DiskView : public BView {
public:
    DiskView();
    virtual ~DiskView();
    
    virtual void AttachedToWindow();
    virtual void DetachedFromWindow();
    virtual void MessageReceived(BMessage* message);
    virtual void Pulse();
    virtual void Draw(BRect updateRect);

private:
    static int32 UpdateThread(void* data);
    void UpdateData(BMessage* message);
    static status_t GetDiskInfo(BVolume& volume, DiskInfo& info);
    
    BBox* fDiskInfoBox;
    BColumnListView* fDiskListView;
    
    BLocker fLocker;
	std::map<dev_t, BRow*> fDeviceRowMap;

    thread_id fUpdateThread;
    sem_id fScanSem;
    volatile bool fTerminated;
};

#endif // DISKVIEW_H
