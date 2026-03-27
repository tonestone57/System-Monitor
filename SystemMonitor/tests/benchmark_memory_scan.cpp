#include <iostream>
#include <map>
#include <vector>
#include <string.h>
#include <sys/time.h>

#include "HaikuMocks.h"

// Globals to track syscalls
long gAreaSyscallCount = 0;

struct area_info {
    size_t ram_size;
};

struct team_info {
    team_id team;
    int32 area_count;
};

// Override mock get_next_area_info for our benchmark
status_t get_next_area_info(team_id team, ssize_t *cookie, area_info *info) {
    gAreaSyscallCount++;
    if (*cookie >= 50) return B_ERROR; // simulate 50 areas per team
    info->ram_size = 4096;
    (*cookie)++;
    return B_OK;
}

struct CachedTeamInfo {
    int32 cachedAreaCount;
    int32 memoryGeneration;
    size_t memoryUsage;
};

int main() {
    std::map<team_id, CachedTeamInfo> fCachedTeamInfo;
    const int numTeams = 200;
    const int numGenerations = 30;
    const int kMemoryCacheGenerations = 10;

    // Initialize cache
    for (int i = 0; i < numTeams; ++i) {
        fCachedTeamInfo[i] = {-1, 0, 0};
    }

    std::cout << "--- Baseline (With Cache Expiration) ---" << std::endl;
    long totalSyscallsBaseline = 0;

    for (int gen = 1; gen <= numGenerations; ++gen) {
        long syscallsThisGen = 0;
        for (int i = 0; i < numTeams; ++i) {
            team_info teamInfo;
            teamInfo.team = i;
            teamInfo.area_count = 50; // Mock 50 areas
            bigtime_t teamActiveTimeDelta = (i % 5 == 0) ? 100 : 0; // 20% active processes

            bool memoryNeedsUpdate = true;
            auto& cachedInfo = fCachedTeamInfo[i];

            if (cachedInfo.cachedAreaCount == teamInfo.area_count &&
                (gen - cachedInfo.memoryGeneration < kMemoryCacheGenerations)) {
                memoryNeedsUpdate = false;
            }

            if (memoryNeedsUpdate) {
                area_info areaInfo;
                ssize_t areaCookie = 0;
                while (get_next_area_info(teamInfo.team, &areaCookie, &areaInfo) == B_OK) {
                    syscallsThisGen++;
                }
                cachedInfo.cachedAreaCount = teamInfo.area_count;
                cachedInfo.memoryGeneration = gen;
            }
        }
        totalSyscallsBaseline += syscallsThisGen;
        if (gen % 5 == 0 || syscallsThisGen > 0) {
            std::cout << "Generation " << gen << " syscalls: " << syscallsThisGen << std::endl;
        }
    }

    std::cout << "Total Syscalls (Baseline): " << totalSyscallsBaseline << std::endl;

    // Reset
    for (int i = 0; i < numTeams; ++i) {
        fCachedTeamInfo[i] = {-1, 0, 0};
    }
    gAreaSyscallCount = 0;

    std::cout << "\n--- Optimized (Idle Skip) ---" << std::endl;
    long totalSyscallsOptimized = 0;

    for (int gen = 1; gen <= numGenerations; ++gen) {
        long syscallsThisGen = 0;
        for (int i = 0; i < numTeams; ++i) {
            team_info teamInfo;
            teamInfo.team = i;
            teamInfo.area_count = 50; // Mock 50 areas
            bigtime_t teamActiveTimeDelta = (i % 5 == 0) ? 100 : 0; // 20% active processes

            bool memoryNeedsUpdate = true;
            auto& cachedInfo = fCachedTeamInfo[i];

            // OPTIMIZATION: Idle Check + Throttle
            if (cachedInfo.cachedAreaCount == teamInfo.area_count) {
                if (teamActiveTimeDelta == 0) {
                    memoryNeedsUpdate = false;
                } else if (gen - cachedInfo.memoryGeneration < kMemoryCacheGenerations) {
                    memoryNeedsUpdate = false;
                }
            }

            if (memoryNeedsUpdate) {
                area_info areaInfo;
                ssize_t areaCookie = 0;
                while (get_next_area_info(teamInfo.team, &areaCookie, &areaInfo) == B_OK) {
                    syscallsThisGen++;
                }
                cachedInfo.cachedAreaCount = teamInfo.area_count;
                cachedInfo.memoryGeneration = gen;
            }
        }
        totalSyscallsOptimized += syscallsThisGen;
        if (gen % 5 == 0 || syscallsThisGen > 0) {
            std::cout << "Generation " << gen << " syscalls: " << syscallsThisGen << std::endl;
        }
    }

    std::cout << "Total Syscalls (Optimized): " << totalSyscallsOptimized << std::endl;
    std::cout << "\nImprovement: " << ((double)(totalSyscallsBaseline - totalSyscallsOptimized) / totalSyscallsBaseline) * 100 << "% fewer system calls." << std::endl;

    return 0;
}
