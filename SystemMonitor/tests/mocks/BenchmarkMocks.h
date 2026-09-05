#ifndef BENCHMARK_MOCKS_H
#define BENCHMARK_MOCKS_H

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <cstring>
#include <cstdint>
#include <cstdio>

// --- Mock Haiku API for Benchmark ---

typedef int32_t team_id;
typedef int32_t thread_id;
typedef int64_t bigtime_t;
typedef int32_t status_t;
typedef uint32_t uid_t;

#ifndef B_OK
const status_t B_OK = 0;
#endif

#ifndef B_ERROR
const status_t B_ERROR = -1;
#endif

#ifndef B_OS_NAME_LENGTH
#define B_OS_NAME_LENGTH 32
#endif

#ifndef B_TEAM_USAGE_SELF
#define B_TEAM_USAGE_SELF 0
#endif

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

inline std::vector<MockTeam>& GetMockTeams() {
    static std::vector<MockTeam> teams;
    return teams;
}
#define gTeams GetMockTeams()

inline long& GetSyscallCount() {
    static long count = 0;
    return count;
}
#define gSyscallCount GetSyscallCount()

// Helper to populate teams
inline void SetupMockTeams() {
    gTeams.clear();
    // Kernel (Team 1)
    MockTeam kernel;
    kernel.id = 1;
    kernel.name = "kernel_team";
    kernel.user_time = 0;
    kernel.kernel_time = 0;
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
inline status_t get_next_team_info(int32_t *cookie, team_info *info) {
    gSyscallCount++;
    if (*cookie >= (int32_t)gTeams.size()) return B_ERROR;
    const auto& t = gTeams[*cookie];
    info->team = t.id;
    snprintf(info->name, B_OS_NAME_LENGTH, "%s", t.name.c_str());
    info->thread_count = t.threads.size();
    info->area_count = 1;
    info->uid = 0;
    (*cookie)++;
    return B_OK;
}

inline status_t get_next_thread_info(team_id team, int32_t *cookie, thread_info *info) {
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

inline status_t get_thread_info(thread_id thread, thread_info *info) {
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

inline status_t get_team_usage_info(team_id team, int32_t who, team_usage_info *info) {
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

#endif // BENCHMARK_MOCKS_H
