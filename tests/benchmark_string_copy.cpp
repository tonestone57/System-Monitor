#include <iostream>
#include <string>
#include <vector>
#include <chrono>

// Mock BString
class BString : public std::string {
public:
    using std::string::string;
    BString() : std::string() {}
    BString(const char* s) : std::string(s ? s : "") {}
    const char* String() const { return c_str(); }
};

BString ReturnByValue(const BString& input) {
    return input;
}

const BString& ReturnByRef(const BString& ref) {
    return ref;
}

int main() {
    BString staticStr("user_1000");

    auto start = std::chrono::high_resolution_clock::now();
    for(int i=0; i<1000000; ++i) {
        BString s = ReturnByValue(staticStr);
        volatile const char* c = s.String();
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "By Value: " << std::chrono::duration<double, std::milli>(end-start).count() << " ms" << std::endl;

    start = std::chrono::high_resolution_clock::now();
    for(int i=0; i<1000000; ++i) {
        const BString& s = ReturnByRef(staticStr);
        volatile const char* c = s.String();
    }
    end = std::chrono::high_resolution_clock::now();
    std::cout << "By Ref: " << std::chrono::duration<double, std::milli>(end-start).count() << " ms" << std::endl;
}
