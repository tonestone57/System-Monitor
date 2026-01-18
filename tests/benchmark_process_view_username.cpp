#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>
#include <chrono>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <stdio.h>

// Mock Haiku types
class BString : public std::string {
public:
    using std::string::string;
    BString() : std::string() {}
    BString(const char* s) : std::string(s ? s : "") {}
    BString& operator<<(int v) { append(std::to_string(v)); return *this; }
    const char* String() const { return c_str(); }
};

// Global mock cache to simulate the member variable
std::unordered_map<uid_t, BString> fUserNameCache;

// Mock getpwuid
struct passwd* mock_getpwuid(uid_t uid) {
    // Simulate delay
    usleep(1000); // 1ms
    static struct passwd pw;
    static char nameBuffer[32];
    sprintf(nameBuffer, "user_%d", uid);
    pw.pw_name = nameBuffer;
    return &pw;
}

// Logic copied from ProcessView.cpp
BString GetUserName(uid_t uid) {
    if (fUserNameCache.count(uid) > 0) {
        return fUserNameCache[uid];
    }

    struct passwd* pw = mock_getpwuid(uid);
    BString name;
    if (pw) {
        name = pw->pw_name;
    } else {
        name << uid;
    }
    fUserNameCache[uid] = name;
    return name;
}

int main() {
    // Benchmark
    auto start = std::chrono::high_resolution_clock::now();

    // Simulate 100 updates, each checking same UID
    for (int i = 0; i < 100; ++i) {
        GetUserName(1000);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;

    std::cout << "Time for 100 calls (same UID): " << elapsed.count() << " ms" << std::endl;

    // Expectation: First call takes ~1ms, rest take ~0ms. Total ~1ms.
    // If not cached: 100 * 1ms = 100ms.

    if (elapsed.count() > 50) {
        std::cout << "FAIL: Too slow, caching not working." << std::endl;
        return 1;
    } else {
        std::cout << "PASS: Fast, caching is working." << std::endl;
        return 0;
    }
}
