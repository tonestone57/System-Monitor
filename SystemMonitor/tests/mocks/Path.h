#ifndef MOCK_PATH_H
#define MOCK_PATH_H
class BPath {
public:
    const char* Path() const { return "/"; }
    int InitCheck() { return 0; }
    const char* Leaf() const { return ""; }
    int Append(const char*) { return 0; }
};
#endif
