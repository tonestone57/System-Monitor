#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <cstring>
#include <cstdint>
#include "BenchmarkMocks.h"

// --- Benchmark Logic ---

struct CachedTeamInfo {
    bigtime_t cpuTime;
    thread_id lastRunningThread;
};

std::map<team_id, CachedTeamInfo> fCachedTeamInfo;

void RunBenchmark(bool useSkipScan, bool useLastRunningThread) {
    gSyscallCount = 0;
    int32_t cookie = 0;
    team_info teamInfo;

    while (get_next_team_info(&cookie, &teamInfo) == B_OK) {
        bool cached = false;
        CachedTeamInfo* cachedInfo = nullptr;
        if (fCachedTeamInfo.count(teamInfo.team)) {
            cached = true;
            cachedInfo = &fCachedTeamInfo[teamInfo.team];
        } else {
             fCachedTeamInfo[teamInfo.team] = CachedTeamInfo{0, -1};
             cachedInfo = &fCachedTeamInfo[teamInfo.team];
        }

        // Simulate logic
        if (teamInfo.team == 1) {
            // Kernel logic (always scans)
             int32_t tCookie = 0;
             thread_info tInfo;
             while (get_next_thread_info(teamInfo.team, &tCookie, &tInfo) == B_OK) {}
        } else {
            team_usage_info usageInfo;
            bigtime_t teamActiveTimeDelta = 0;
            if (get_team_usage_info(teamInfo.team, B_TEAM_USAGE_SELF, &usageInfo) == B_OK) {
                bigtime_t currentTeamTime = usageInfo.user_time + usageInfo.kernel_time;
                if (cached) {
                    teamActiveTimeDelta = currentTeamTime - cachedInfo->cpuTime;
                    if (teamActiveTimeDelta < 0) teamActiveTimeDelta = 0;
                }
                cachedInfo->cpuTime = currentTeamTime;
            }

            // OPTIMIZATION 1: Skip scan if idle
            bool skipScan = false;
            if (useSkipScan && cached && teamActiveTimeDelta == 0) {
                skipScan = true;
            }

            // OPTIMIZATION 2: Check last running thread
            if (!skipScan && useLastRunningThread && cached && cachedInfo->lastRunningThread != -1) {
                 thread_info lastInfo;
                 if (get_thread_info(cachedInfo->lastRunningThread, &lastInfo) == B_OK
                     && lastInfo.team == teamInfo.team
                     && lastInfo.state == B_THREAD_RUNNING) {
                     skipScan = true; // Found running, skipping scan!
                 }
            }

            if (!skipScan) {
                int32_t tCookie = 0;
                thread_info tInfo;
                while (get_next_thread_info(teamInfo.team, &tCookie, &tInfo) == B_OK) {
                    if (tInfo.state == B_THREAD_RUNNING) {
                        if (useLastRunningThread) {
                            cachedInfo->lastRunningThread = tInfo.thread;
                        }
                        break;
                    }
                }
            }
        }
    }
}

int main() {
    SetupMockTeams();

    // 1. Warmup (populate cache) with FULL optimization enabled to populate lastRunningThread
    // Note: To be fair, we should warmup differently for different runs, but cache population is same.
    // However, to measure "steady state", we should run once to populate cache.
    RunBenchmark(true, true);

    // 2. Advance time for Active teams only
    for(auto& t : gTeams) {
        if (t.id > 90) { // Active
            t.user_time += 100; // Simulated CPU usage
        }
    }

    // Reset Syscall Count
    gSyscallCount = 0;

    // 3. Measure Baseline (SkipScan=ON, LastThread=OFF)
    // This represents the state BEFORE this task.
    long syscallsBaseline = 0;
    {
        // Copy cache state to ensure fair comparison?
        // No, let's just run it. But wait, lastRunningThread won't be used.
        auto cacheSnapshot = fCachedTeamInfo;
        RunBenchmark(true, false);
        syscallsBaseline = gSyscallCount;
        fCachedTeamInfo = cacheSnapshot; // Restore cache
    }

    // 4. Measure New Optimization (SkipScan=ON, LastThread=ON)
    long syscallsOptimized = 0;
    {
        gSyscallCount = 0;
        RunBenchmark(true, true);
        syscallsOptimized = gSyscallCount;
    }

    std::cout << "Baseline (Existing Optimization): " << syscallsBaseline << std::endl;
    std::cout << "New Optimization (Last Thread Cache): " << syscallsOptimized << std::endl;
    std::cout << "Reduction: " << (syscallsBaseline - syscallsOptimized) << " ("
              << (100.0 * (syscallsBaseline - syscallsOptimized) / syscallsBaseline) << "%)" << std::endl;

    return 0;
}
