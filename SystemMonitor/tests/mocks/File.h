#ifndef MOCK_FILE_H
#define MOCK_FILE_H
class BFile {
public:
    BFile() {}
    BFile(const char*, int) {}
    int SetTo(const char*, int) { return 0; }
    int InitCheck() { return 0; }
    int Read(void*, int) { return 0; }
};
#define B_READ_ONLY 0
#endif
