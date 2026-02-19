#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <cstring>
#include <cstdint>

// --- Mock Haiku API for Benchmark ---

typedef int32_t team_id;
typedef int32_t thread_id;
typedef int64_t bigtime_t;
typedef int32_t status_t;
typedef uint32_t uid_t;

const status_t B_OK = 0;
const status_t B_ERROR = -1;

#define B_OS_NAME_LENGTH 32
#define B_TEAM_USAGE_SELF 0

enum thread_state {
    B_THREAD_RUNNING = 1,
    B_THREAD_READY,
    B_THREAD_RECEIVING,
    B_THREAD_ASLEEP,
    B_THREAD_SUSPENDED,
    B_THREAD_WAITING
};

struct team_info {
    team_id team;
    char name[B_OS_NAME_LENGTH];
    char args[64];
    int32_t thread_count;
    int32_t area_count;
    uid_t uid;
};

struct thread_info {
    thread_id thread;
    team_id team;
    char name[B_OS_NAME_LENGTH];
    thread_state state;
    int32_t priority;
    bigtime_t user_time;
    bigtime_t kernel_time;
};

struct team_usage_info {
    bigtime_t user_time;
    bigtime_t kernel_time;
};

// Global Mock State
struct MockThread {
    thread_id id;
    thread_state state;
};

struct MockTeam {
    team_id id;
    std::string name;
    std::vector<MockThread> threads;
    bigtime_t user_time;
    bigtime_t kernel_time;
};

std::vector<MockTeam> gTeams;
long gSyscallCount = 0;

// Helper to populate teams
void SetupMockTeams() {
    gTeams.clear();
    // Kernel (Team 1)
    MockTeam kernel;
    kernel.id = 1;
    kernel.name = "kernel_team";
    for(int i=0; i<50; ++i) kernel.threads.push_back({(thread_id)(1000+i), B_THREAD_RUNNING});
    gTeams.push_back(kernel);

    // Idle Daemons (Teams 2-90)
    for(int i=2; i<=90; ++i) {
        MockTeam t;
        t.id = i;
        t.name = "daemon_" + std::to_string(i);
        t.user_time = 1000; // Constant
        t.kernel_time = 500;
        for(int j=0; j<3; ++j) {
            t.threads.push_back({(thread_id)(i*100+j), B_THREAD_ASLEEP});
        }
        gTeams.push_back(t);
    }

    // Active Apps (Teams 91-100)
    for(int i=91; i<=100; ++i) {
        MockTeam t;
        t.id = i;
        t.name = "app_" + std::to_string(i);
        t.user_time = 5000;
        t.kernel_time = 2000;
        // One running thread (thread 5), rest ready/waiting
        // Add some READY threads first
        for(int j=0; j<5; ++j) {
            t.threads.push_back({(thread_id)(i*100+j), B_THREAD_READY});
        }
        // The RUNNING thread
        t.threads.push_back({(thread_id)(i*100+5), B_THREAD_RUNNING});
        // More READY threads
        for(int j=6; j<10; ++j) {
            t.threads.push_back({(thread_id)(i*100+j), B_THREAD_READY});
        }
        gTeams.push_back(t);
    }
}

// Implement Mock APIs
status_t get_next_team_info(int32_t *cookie, team_info *info) {
    gSyscallCount++;
    if (*cookie >= (int32_t)gTeams.size()) return B_ERROR;
    const auto& t = gTeams[*cookie];
    info->team = t.id;
    strncpy(info->name, t.name.c_str(), B_OS_NAME_LENGTH);
    info->thread_count = t.threads.size();
    info->area_count = 1;
    info->uid = 0;
    (*cookie)++;
    return B_OK;
}

status_t get_next_thread_info(team_id team, int32_t *cookie, thread_info *info) {
    gSyscallCount++;
    // Find team
    const MockTeam* target = nullptr;
    for(const auto& t : gTeams) {
        if (t.id == team) {
            target = &t;
            break;
        }
    }
    if (!target) return B_ERROR;

    if (*cookie >= (int32_t)target->threads.size()) return B_ERROR;
    const auto& th = target->threads[*cookie];
    info->thread = th.id;
    info->team = team;
    info->state = th.state;
    (*cookie)++;
    return B_OK;
}

status_t get_thread_info(thread_id thread, thread_info *info) {
    gSyscallCount++;
    for(const auto& t : gTeams) {
        for(const auto& th : t.threads) {
            if (th.id == thread) {
                info->thread = th.id;
                info->team = t.id;
                info->state = th.state;
                return B_OK;
            }
        }
    }
    return B_ERROR;
}

status_t get_team_usage_info(team_id team, int32_t who, team_usage_info *info) {
    gSyscallCount++;
    for(const auto& t : gTeams) {
        if (t.id == team) {
            info->user_time = t.user_time;
            info->kernel_time = t.kernel_time;
            return B_OK;
        }
    }
    return B_ERROR;
}

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
            bool isRunning = false;

            if (useSkipScan && cached && teamActiveTimeDelta == 0) {
                skipScan = true;
            }

            // OPTIMIZATION 3 (NEW): Skip scan if active
            if (!skipScan && useActiveSkip && cached && teamActiveTimeDelta > 0) {
                skipScan = true;
                isRunning = true;
            }

            // OPTIMIZATION 2: Check last running thread
            if (!skipScan && useLastRunningThread && cached && cachedInfo->lastRunningThread != -1) {
                 thread_info lastInfo;
                 if (get_thread_info(cachedInfo->lastRunningThread, &lastInfo) == B_OK
                     && lastInfo.team == teamInfo.team
                     && lastInfo.state == B_THREAD_RUNNING) {
                     skipScan = true; // Found running, skipping scan!
                     isRunning = true;
                 }
            }

            if (!skipScan) {
                int32_t tCookie = 0;
                thread_info tInfo;
                bool isReady = false;
                while (get_next_thread_info(teamInfo.team, &tCookie, &tInfo) == B_OK) {
                    if (tInfo.state == B_THREAD_RUNNING) {
                        isRunning = true;
                        if (useLastRunningThread) {
                            cachedInfo->lastRunningThread = tInfo.thread;
                        }
                        break;
                    }
                    if (tInfo.state == B_THREAD_READY) isReady = true;
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
