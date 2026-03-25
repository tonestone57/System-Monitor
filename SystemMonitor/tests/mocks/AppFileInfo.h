#ifndef MOCK_APP_FILE_INFO_H
#define MOCK_APP_FILE_INFO_H
class BFile;
struct version_info {
    int major;
    int middle;
    int minor;
    int prerelease;
    char short_info[64];
};
class BAppFileInfo {
public:
    int SetTo(BFile*) { return 0; }
    int GetVersionInfo(version_info*, int) { return 0; }
};
#define B_APP_VERSION_KIND 0
#endif
