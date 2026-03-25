#include <iostream>
#include <chrono>

// Mock struct to simulate system_info for benchmarking
struct system_info {
    int cpu_count;
};

// Mock function for get_system_info
int get_system_info(system_info* info) {
    if (info) {
        info->cpu_count = 8;
    }
    return 0; // B_OK
}

// Global cached value
static int sCachedCpuCount = 0;

int GetCachedCpuCount() {
    if (sCachedCpuCount == 0) {
        system_info sysInfo;
        get_system_info(&sysInfo);
        sCachedCpuCount = sysInfo.cpu_count;
    }
    return sCachedCpuCount;
}

void benchmark_no_cache() {
    auto start = std::chrono::high_resolution_clock::now();
    long long total = 0;

    // Simulate what ProcessView::UpdateThread does over many iterations
    for (int i = 0; i < 10000000; i++) {
        system_info sysInfo;
        get_system_info(&sysInfo);

        // Simulating the use of cpu_count
        long long systemTimeDelta = 1000; // Simulated delta
        float totalPossibleCoreTime = sysInfo.cpu_count * systemTimeDelta;
        if (totalPossibleCoreTime <= 0) totalPossibleCoreTime = 1.0f;

        total += static_cast<long long>(totalPossibleCoreTime);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "No cache (10,000,000 iterations) took: " << duration.count() << " microseconds (Result: " << total << ")" << std::endl;
}

void benchmark_cache() {
    auto start = std::chrono::high_resolution_clock::now();
    long long total = 0;

    // Reset cache for fair comparison if it was somehow used before
    // (though in reality it's initialized to 0 once)
    sCachedCpuCount = 0;

    for (int i = 0; i < 10000000; i++) {
        // Simulating the optimized version using cached CPU count
        long long systemTimeDelta = 1000; // Simulated delta
        float totalPossibleCoreTime = GetCachedCpuCount() * systemTimeDelta;
        if (totalPossibleCoreTime <= 0) totalPossibleCoreTime = 1.0f;

        total += static_cast<long long>(totalPossibleCoreTime);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Cache    (10,000,000 iterations) took: " << duration.count() << " microseconds (Result: " << total << ")" << std::endl;
}

int main() {
    benchmark_no_cache();
    benchmark_cache();
    return 0;
}
