#include <iostream>
#include <chrono>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <cstdio>

class BString {
public:
    std::string s;
    void SetToFormat(const char* fmt, int i) {
        char buf[256];
        snprintf(buf, sizeof(buf), fmt, i);
        s = buf;
    }
    const char* String() const { return s.c_str(); }
};

std::string GetBatteryCapacityUnoptimized() {
    for (int i = 0; i < 4; i++) {
        BString path;
        if (i < 3) {
            path.SetToFormat("/dev/non_existent_path_%d", i);
        } else {
            path.SetToFormat("/dev/null", i);
        }

        int batFd = open(path.String(), O_RDONLY);
        if (batFd >= 0) {
            close(batFd);
            return "100%";
        }
    }
    return "Unknown";
}

std::string GetBatteryCapacityOptimized() {
    static int sCachedBatteryIndex = -1;

    if (sCachedBatteryIndex >= 0) {
        BString path;
        path.SetToFormat("/dev/null", sCachedBatteryIndex);
        int batFd = open(path.String(), O_RDONLY);
        if (batFd >= 0) {
            close(batFd);
            return "100%";
        }
        sCachedBatteryIndex = -1;
    }

    for (int i = 0; i < 4; i++) {
        BString path;
        if (i < 3) {
            path.SetToFormat("/dev/non_existent_path_%d", i);
        } else {
            path.SetToFormat("/dev/null", i);
        }

        int batFd = open(path.String(), O_RDONLY);
        if (batFd >= 0) {
            sCachedBatteryIndex = i;
            close(batFd);
            return "100%";
        }
    }
    return "Unknown";
}

int main() {
    const int iterations = 10000;

    auto start_unopt = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        GetBatteryCapacityUnoptimized();
    }
    auto end_unopt = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::micro> unopt_time = end_unopt - start_unopt;

    auto start_opt = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        GetBatteryCapacityOptimized();
    }
    auto end_opt = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::micro> opt_time = end_opt - start_opt;

    std::cout << "Baseline: " << unopt_time.count() / iterations << " us/call" << std::endl;
    std::cout << "Optimized: " << opt_time.count() / iterations << " us/call" << std::endl;
    std::cout << "Improvement: " << (unopt_time.count() / opt_time.count()) << "x faster" << std::endl;

    return 0;
}
