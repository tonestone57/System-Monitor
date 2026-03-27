#include <iostream>
#include <vector>
#include <cstdint>
#include <cstring>

// --- Mock Haiku API for Benchmark ---

typedef int32_t team_id;
typedef int32_t thread_id;
typedef int32_t status_t;

const status_t B_OK = 0;
const status_t B_ERROR = -1;

#define B_OS_NAME_LENGTH 32

struct thread_info {
    thread_id thread;
    team_id team;
    char name[B_OS_NAME_LENGTH];
    int32_t priority;
};

// Global Mock State
struct MockThread {
    thread_id id;
    int32_t priority;
};

struct MockTeam {
    team_id id;
    std::vector<MockThread> threads;
};

std::vector<MockTeam> gTeams;
long gSyscallCount = 0;
long gSetPriorityCalls = 0;
long gGetNextThreadCalls = 0;

void SetupMockTeams(int numThreads, int32_t initialPriority) {
    gTeams.clear();
    MockTeam team;
    team.id = 100;
    for (int i = 0; i < numThreads; ++i) {
        team.threads.push_back({(thread_id)(1000 + i), initialPriority});
    }
    gTeams.push_back(team);
}

status_t get_next_thread_info(team_id team, int32_t *cookie, thread_info *info) {
    gSyscallCount++;
    gGetNextThreadCalls++;

    const MockTeam* target = nullptr;
    for (const auto& t : gTeams) {
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
    info->priority = th.priority;
    (*cookie)++;
    return B_OK;
}

status_t set_thread_priority(thread_id thread, int32_t priority) {
    gSyscallCount++;
    gSetPriorityCalls++;

    for (auto& t : gTeams) {
        for (auto& th : t.threads) {
            if (th.id == thread) {
                th.priority = priority;
                return B_OK;
            }
        }
    }
    return B_ERROR;
}

// --- Benchmark Logic ---

void SetProcessPriority_Original(team_id team, int32_t priority) {
    thread_info tInfo;
    int32_t cookie = 0;
    while (get_next_thread_info(team, &cookie, &tInfo) == B_OK) {
        set_thread_priority(tInfo.thread, priority);
    }
}

void SetProcessPriority_Optimized(team_id team, int32_t priority) {
    thread_info tInfo;
    int32_t cookie = 0;
    while (get_next_thread_info(team, &cookie, &tInfo) == B_OK) {
        if (tInfo.priority != priority) {
            set_thread_priority(tInfo.thread, priority);
        }
    }
}

int main() {
    const int numThreads = 100;
    const int32_t priorityA = 10;
    const int32_t priorityB = 20;

    std::cout << "--- Benchmark: Setting priority for " << numThreads << " threads ---" << std::endl;

    // 1. Baseline: Change priority for all threads
    SetupMockTeams(numThreads, priorityA);
    gSyscallCount = 0;
    gGetNextThreadCalls = 0;
    gSetPriorityCalls = 0;
    SetProcessPriority_Original(100, priorityB);
    std::cout << "Original (Priority Change Needed):" << std::endl;
    std::cout << "  Total Syscalls: " << gSyscallCount << std::endl;
    std::cout << "  get_next_thread_info: " << gGetNextThreadCalls << std::endl;
    std::cout << "  set_thread_priority: " << gSetPriorityCalls << std::endl;

    // 2. Baseline: Priority already set (Redundant calls)
    SetupMockTeams(numThreads, priorityB);
    gSyscallCount = 0;
    gGetNextThreadCalls = 0;
    gSetPriorityCalls = 0;
    SetProcessPriority_Original(100, priorityB);
    std::cout << "Original (Already at Target Priority):" << std::endl;
    std::cout << "  Total Syscalls: " << gSyscallCount << std::endl;
    std::cout << "  get_next_thread_info: " << gGetNextThreadCalls << std::endl;
    std::cout << "  set_thread_priority: " << gSetPriorityCalls << std::endl;

    // 3. Optimized: Change priority for all threads
    SetupMockTeams(numThreads, priorityA);
    gSyscallCount = 0;
    gGetNextThreadCalls = 0;
    gSetPriorityCalls = 0;
    SetProcessPriority_Optimized(100, priorityB);
    std::cout << "Optimized (Priority Change Needed):" << std::endl;
    std::cout << "  Total Syscalls: " << gSyscallCount << std::endl;
    std::cout << "  get_next_thread_info: " << gGetNextThreadCalls << std::endl;
    std::cout << "  set_thread_priority: " << gSetPriorityCalls << std::endl;

    // 4. Optimized: Priority already set (No redundant calls)
    SetupMockTeams(numThreads, priorityB);
    gSyscallCount = 0;
    gGetNextThreadCalls = 0;
    gSetPriorityCalls = 0;
    SetProcessPriority_Optimized(100, priorityB);
    std::cout << "Optimized (Already at Target Priority):" << std::endl;
    std::cout << "  Total Syscalls: " << gSyscallCount << std::endl;
    std::cout << "  get_next_thread_info: " << gGetNextThreadCalls << std::endl;
    std::cout << "  set_thread_priority: " << gSetPriorityCalls << std::endl;

    return 0;
}
