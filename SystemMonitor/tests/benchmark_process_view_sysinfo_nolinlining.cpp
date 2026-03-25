#include <iostream>
#include <chrono>

struct system_info {
    int cpu_count;
};

// Prevent inlining to simulate the real OS call overhead
__attribute__((noinline)) int get_system_info_mock(system_info* info) {
    if (info) {
        info->cpu_count = 8;
    }
    // Simulate some work an OS would do
    volatile int dummy = 0;
    for(int i = 0; i < 50; i++) dummy += i;

    return 0; // B_OK
}

static int sCachedCpuCount = 0;

__attribute__((noinline)) int GetCachedCpuCount() {
    if (sCachedCpuCount == 0) {
        system_info sysInfo;
        get_system_info_mock(&sysInfo);
        sCachedCpuCount = sysInfo.cpu_count;
    }
    return sCachedCpuCount;
}

void benchmark_no_cache() {
    auto start = std::chrono::high_resolution_clock::now();
    long long total = 0;

    for (int i = 0; i < 1000000; i++) {
        system_info sysInfo;
        get_system_info_mock(&sysInfo);

        long long systemTimeDelta = 1000;
        float totalPossibleCoreTime = sysInfo.cpu_count * systemTimeDelta;
        if (totalPossibleCoreTime <= 0) totalPossibleCoreTime = 1.0f;

        total += static_cast<long long>(totalPossibleCoreTime);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "No cache (1,000,000 iterations) took: " << duration.count() << " microseconds (Result: " << total << ")" << std::endl;
}

void benchmark_cache() {
    auto start = std::chrono::high_resolution_clock::now();
    long long total = 0;

    sCachedCpuCount = 0;

    for (int i = 0; i < 1000000; i++) {
        long long systemTimeDelta = 1000;
        float totalPossibleCoreTime = GetCachedCpuCount() * systemTimeDelta;
        if (totalPossibleCoreTime <= 0) totalPossibleCoreTime = 1.0f;

        total += static_cast<long long>(totalPossibleCoreTime);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Cache    (1,000,000 iterations) took: " << duration.count() << " microseconds (Result: " << total << ")" << std::endl;
}

int main() {
    benchmark_no_cache();
    benchmark_cache();
    return 0;
}
