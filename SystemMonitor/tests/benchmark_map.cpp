#include <unordered_map>
#include <vector>
#include <iostream>
#include <chrono>
#include <random>
#include <cstring>
#include <cstdio>

using namespace std;

using thread_id = int32_t;
using bigtime_t = int64_t;

struct ThreadInfo {
    thread_id thread;
    bigtime_t user_time;
    bigtime_t kernel_time;
    char name[32];
};

int main() {
    const int num_threads = 2000;
    const int iterations = 10000;

    vector<ThreadInfo> threads;
    for (int i = 0; i < num_threads; ++i) {
        ThreadInfo info;
        info.thread = i;
        info.user_time = 1000 + i;
        info.kernel_time = 500 + i;
        snprintf(info.name, sizeof(info.name), "thread_%d", i);
        threads.push_back(info);
    }

    // Baseline
    {
        unordered_map<thread_id, bigtime_t> threadTimeMap;
        // Pre-fill
        for (const auto& t : threads) {
            threadTimeMap[t.thread] = 0;
        }

        auto start = chrono::high_resolution_clock::now();

        volatile long long totalDelta = 0; // Prevent optimization

        for (int k = 0; k < iterations; ++k) {
            for (const auto& tInfo : threads) {
                bigtime_t threadTime = tInfo.user_time + tInfo.kernel_time + k; // change time

                if (threadTimeMap.count(tInfo.thread)) {
                    bigtime_t threadTimeDelta = threadTime - threadTimeMap[tInfo.thread];
                    if (threadTimeDelta < 0) threadTimeDelta = 0;
                    if (strstr(tInfo.name, "idle thread") == NULL) {
                         totalDelta += threadTimeDelta;
                    }
                }
                threadTimeMap[tInfo.thread] = threadTime;
            }
        }

        auto end = chrono::high_resolution_clock::now();
        cout << "Baseline duration: "
             << chrono::duration_cast<chrono::microseconds>(end - start).count()
             << " us" << endl;
    }

    // Optimized
    {
        unordered_map<thread_id, bigtime_t> threadTimeMap;
        // Pre-fill
        for (const auto& t : threads) {
            threadTimeMap[t.thread] = 0;
        }

        auto start = chrono::high_resolution_clock::now();

        volatile long long totalDelta = 0;

        for (int k = 0; k < iterations; ++k) {
            for (const auto& tInfo : threads) {
                bigtime_t threadTime = tInfo.user_time + tInfo.kernel_time + k;

                auto it = threadTimeMap.find(tInfo.thread);
                if (it != threadTimeMap.end()) {
                    bigtime_t threadTimeDelta = threadTime - it->second;
                    if (threadTimeDelta < 0) threadTimeDelta = 0;

                    if (strstr(tInfo.name, "idle thread") == NULL) {
                        totalDelta += threadTimeDelta;
                    }

                    it->second = threadTime;
                } else {
                    threadTimeMap.insert({tInfo.thread, threadTime});
                }
            }
        }

        auto end = chrono::high_resolution_clock::now();
        cout << "Optimized duration: "
             << chrono::duration_cast<chrono::microseconds>(end - start).count()
             << " us" << endl;
    }

    return 0;
}
