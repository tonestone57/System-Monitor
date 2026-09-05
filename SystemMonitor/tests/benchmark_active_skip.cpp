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

void RunBenchmark(bool useSkipScan, bool useLastRunningThread, bool useActiveSkip) {
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

            // OPTIMIZATION 3 (NEW): Skip scan if active
            if (!skipScan && useActiveSkip && cached && teamActiveTimeDelta > 0) {
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

    // 1. Warmup
    fCachedTeamInfo.clear();
    RunBenchmark(true, true, false);

    // 2. Advance time for Active teams
    for(auto& t : gTeams) {
        if (t.id > 90) { // Active
            t.user_time += 100;
            // Change running thread from 5 to 6 to simulate context switch
            // The cached "lastRunningThread" (5) is no longer running.
            // Baseline will fail the check and fall back to scanning.
            for(auto& th : t.threads) {
                if (th.id == (thread_id)(t.id*100+5)) th.state = B_THREAD_READY;
                if (th.id == (thread_id)(t.id*100+6)) th.state = B_THREAD_RUNNING;
            }
        }
    }

    // 3. Baseline: Current Optimization (SkipIdle + LastRunning)
    long syscallsBaseline = 0;
    {
        gSyscallCount = 0;
        RunBenchmark(true, true, false);
        syscallsBaseline = gSyscallCount;
    }

    // 4. New Optimization: (SkipIdle + ActiveSkip)
    // Note: ActiveSkip overrides LastRunning check if effective
    long syscallsNew = 0;
    {
        gSyscallCount = 0;
        RunBenchmark(true, true, true);
        syscallsNew = gSyscallCount;
    }

    std::cout << "Baseline (IdleSkip + LastRun): " << syscallsBaseline << std::endl;
    std::cout << "New (IdleSkip + ActiveSkip):   " << syscallsNew << std::endl;
    std::cout << "Reduction: " << (syscallsBaseline - syscallsNew) << std::endl;

    return 0;
}
