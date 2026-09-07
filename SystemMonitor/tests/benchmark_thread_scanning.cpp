#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <cstring>
#include <cstdint>
#include "mocks/BenchmarkMocks.h"

// --- Benchmark Logic ---

#include <unordered_set>

struct CachedTeamInfo {
    bigtime_t cpuTime;
    thread_id lastRunningThread;
    int32_t lastPriority;
};

std::map<team_id, CachedTeamInfo> fCachedTeamInfo;

void RunBenchmark(bool useSkipScan, bool useLastRunningThread, bool useHiddenSkip, const std::unordered_set<team_id>& visibleTeams) {
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
             fCachedTeamInfo[teamInfo.team] = CachedTeamInfo{0, -1, 10};
             cachedInfo = &fCachedTeamInfo[teamInfo.team];
        }

        int32_t teamPriority = 10;
        bool priorityFound = false;

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

            // OPTIMIZATION 3: Skip scan if non-visible process
            bool isVisible = visibleTeams.find(teamInfo.team) != visibleTeams.end();
            if (useHiddenSkip && cachedInfo != nullptr && !isVisible) {
                skipScan = true;
                teamPriority = cachedInfo->lastPriority;
                priorityFound = true;
            }

            // OPTIMIZATION 2: Check last running thread
            if (skipScan && cached && cachedInfo->lastRunningThread != -1 && !priorityFound) {
                 thread_info lastInfo;
                 if (get_thread_info(cachedInfo->lastRunningThread, &lastInfo) == B_OK
                     && lastInfo.team == teamInfo.team) {
                     teamPriority = lastInfo.priority;
                     priorityFound = true;
                 }
            }

            if (!skipScan || !priorityFound) {
                if (cached && cachedInfo->lastRunningThread != -1) {
                     thread_info lastInfo;
                     if (get_thread_info(cachedInfo->lastRunningThread, &lastInfo) == B_OK
                         && lastInfo.team == teamInfo.team) {
                         if (!priorityFound) {
                             teamPriority = lastInfo.priority;
                             priorityFound = true;
                         }
                         if (lastInfo.state == B_THREAD_RUNNING) {
                             skipScan = true; // Found running, skipping scan!
                         }
                     }
                }

                if (!skipScan || !priorityFound) {
                    int32_t tCookie = 0;
                    thread_info tInfo;
                    while (get_next_thread_info(teamInfo.team, &tCookie, &tInfo) == B_OK) {
                        if (!priorityFound) {
                            teamPriority = tInfo.priority;
                            priorityFound = true;
                        }
                        if (tInfo.state == B_THREAD_RUNNING) {
                            if (useLastRunningThread) {
                                cachedInfo->lastRunningThread = tInfo.thread;
                            }
                            break;
                        }
                    }
                }
            }

            if (priorityFound) {
                cachedInfo->lastPriority = teamPriority;
            }
        }
    }
}

int main() {
    SetupMockTeams();

    // Visible teams: Kernel (1) and active teams 91-95 (so 96-100 active teams are off-screen/hidden)
    std::unordered_set<team_id> visibleTeams;
    visibleTeams.insert(1);
    for (int i = 91; i <= 95; ++i) visibleTeams.insert(i);

    // 1. Warmup (populate cache)
    RunBenchmark(true, true, true, visibleTeams);

    // 2. Advance time for Active teams (both visible 91-95 and hidden 96-100)
    for(auto& t : gTeams) {
        if (t.id > 90) { // Active
            t.user_time += 100; // Simulated CPU usage
        }
    }

    // 3. Measure Baseline (HiddenSkip = OFF)
    long syscallsBaseline = 0;
    {
        auto cacheSnapshot = fCachedTeamInfo;
        RunBenchmark(true, true, false, visibleTeams);
        syscallsBaseline = gSyscallCount;
        fCachedTeamInfo = cacheSnapshot;
    }

    // 4. Measure New Optimization (HiddenSkip = ON)
    long syscallsOptimized = 0;
    {
        gSyscallCount = 0;
        RunBenchmark(true, true, true, visibleTeams);
        syscallsOptimized = gSyscallCount;
    }

    std::cout << "Baseline (Without Non-Visible Skip): " << syscallsBaseline << std::endl;
    std::cout << "New Optimization (With Non-Visible Skip): " << syscallsOptimized << std::endl;
    std::cout << "Reduction: " << (syscallsBaseline - syscallsOptimized) << " ("
              << (100.0 * (syscallsBaseline - syscallsOptimized) / syscallsBaseline) << "%)" << std::endl;

    return 0;
}
