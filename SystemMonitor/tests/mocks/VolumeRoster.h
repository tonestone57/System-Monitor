#ifndef MOCK_VOLUME_ROSTER_H
#define MOCK_VOLUME_ROSTER_H
class BVolume;
class BVolumeRoster {
public:
    int GetBootVolume(BVolume*) { return 0; }
};
#endif
