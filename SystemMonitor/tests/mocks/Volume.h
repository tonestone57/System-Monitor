#ifndef MOCK_VOLUME_H
#define MOCK_VOLUME_H
class BVolume {
public:
    BVolume() {}
    BVolume(int device) {}
    int GetIcon(class BBitmap* icon, int which) const { return -1; }
public:
    int Device() const { return 0; }
};
#endif
