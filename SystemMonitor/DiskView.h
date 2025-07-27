#ifndef DISKVIEW_H
#define DISKVIEW_H

#include <View.h>
#include <Locker.h>
#include <String.h>
#include <Volume.h>

class BBox;
class BColumnListView;

struct DiskInfo {
    BString deviceName;
    BString mountPoint;
    BString fileSystemType;
    uint64 totalSize;
    uint64 freeSize;
};

class DiskView : public BView {
public:
    DiskView(BRect frame);
    virtual ~DiskView();
    
    virtual void AttachedToWindow();
    virtual void Pulse();
    virtual void Draw(BRect updateRect);

private:
    void UpdateData();
    void GetDiskInfo(BVolume& volume, DiskInfo& info);
    BString FormatBytes(uint64 bytes);
    
    BBox* fDiskInfoBox;
    BColumnListView* fDiskListView;
    
    BLocker fLocker;
};

#endif // DISKVIEW_H