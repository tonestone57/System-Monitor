#ifndef MOCK_ENTRY_H
#define MOCK_ENTRY_H
class BPath;
class BEntry {
public:
    int GetPath(BPath*) { return 0; }
};
#endif
