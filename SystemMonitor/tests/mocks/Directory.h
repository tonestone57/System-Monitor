#ifndef MOCK_DIRECTORY_H
#define MOCK_DIRECTORY_H
class BEntry;
class BDirectory {
public:
    BDirectory(const char*) {}
    int InitCheck() { return 0; }
    int GetNextEntry(BEntry*) { return -1; }
};
#endif
