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
    BString deviceName;
    BString mountPoint;
    BString fileSystemType;
    uint64 totalSize;
    uint64 freeSize;
	dev_t deviceID;
};

class DiskView : public BView {
public:
    DiskView();
    virtual ~DiskView();
    
    virtual void AttachedToWindow();
    virtual void Pulse();
    virtual void Draw(BRect updateRect);

private:
    void UpdateData();
    status_t GetDiskInfo(BVolume& volume, DiskInfo& info);
    BString FormatBytes(uint64 bytes);
    
    BBox* fDiskInfoBox;
    BColumnListView* fDiskListView;
    
    BLocker fLocker;
	std::map<dev_t, BRow*> fDeviceRowMap;
};

#endif // DISKVIEW_H