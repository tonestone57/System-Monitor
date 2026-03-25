#ifndef MOCK_FIND_DIRECTORY_H
#define MOCK_FIND_DIRECTORY_H
class BPath;
#define B_SYSTEM_NONPACKAGED_DIRECTORY 1
inline int find_directory(int, BPath*) { return 0; }
#endif
