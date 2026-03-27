#include <cassert>
#include <iostream>
#include <cstring>
#include "HaikuMocks.h"
#include "../ProcessListItem.h"

BFont* be_bold_font = nullptr;

ProcessInfo create_process_info(team_id id, const char* name, const char* user,
                               ProcessState state, float cpu, uint64 mem, uint32 threads) {
    ProcessInfo info;
    info.id = id;
    snprintf(info.name, B_OS_NAME_LENGTH, "%s", name);
    snprintf(info.userName, B_OS_NAME_LENGTH, "%s", user);
    info.state = state;
    info.cpuUsage = cpu;
    info.memoryUsageBytes = mem;
    info.threadCount = threads;
    return info;
}

void test_compare_cpu() {
    ProcessInfo i1 = create_process_info(1, "p1", "u1", PROCESS_STATE_RUNNING, 10.0f, 100, 1);
    ProcessInfo i2 = create_process_info(2, "p2", "u2", PROCESS_STATE_RUNNING, 20.0f, 200, 2);
    ProcessListItem item1(i1, "Running", nullptr, nullptr);
    ProcessListItem item2(i2, "Running", nullptr, nullptr);
    const ProcessListItem* p1 = &item1;
    const ProcessListItem* p2 = &item2;

    // Descending order: i2 (20.0) should be "less" than i1 (10.0) in sorting (top of list)
    // CompareCPU(a, b) returns -1 if a > b
    assert(ProcessListItem::CompareCPU(&p1, &p2) > 0);
    assert(ProcessListItem::CompareCPU(&p2, &p1) < 0);
    assert(ProcessListItem::CompareCPU(&p1, &p1) == 0);
}

void test_compare_pid() {
    ProcessInfo i1 = create_process_info(1, "p1", "u1", PROCESS_STATE_RUNNING, 10.0f, 100, 1);
    ProcessInfo i2 = create_process_info(2, "p2", "u2", PROCESS_STATE_RUNNING, 20.0f, 200, 2);
    ProcessListItem item1(i1, "Running", nullptr, nullptr);
    ProcessListItem item2(i2, "Running", nullptr, nullptr);
    const ProcessListItem* p1 = &item1;
    const ProcessListItem* p2 = &item2;

    // Ascending order
    assert(ProcessListItem::ComparePID(&p1, &p2) < 0);
    assert(ProcessListItem::ComparePID(&p2, &p1) > 0);
    assert(ProcessListItem::ComparePID(&p1, &p1) == 0);
}

void test_compare_name() {
    ProcessInfo i1 = create_process_info(1, "abc", "u1", PROCESS_STATE_RUNNING, 10.0f, 100, 1);
    ProcessInfo i2 = create_process_info(2, "DEF", "u2", PROCESS_STATE_RUNNING, 20.0f, 200, 2);
    ProcessListItem item1(i1, "Running", nullptr, nullptr);
    ProcessListItem item2(i2, "Running", nullptr, nullptr);
    const ProcessListItem* p1 = &item1;
    const ProcessListItem* p2 = &item2;

    // Case-insensitive ascending (strcasecmp)
    assert(ProcessListItem::CompareName(&p1, &p2) < 0);
    assert(ProcessListItem::CompareName(&p2, &p1) > 0);
    assert(ProcessListItem::CompareName(&p1, &p1) == 0);
}

void test_compare_mem() {
    ProcessInfo i1 = create_process_info(1, "p1", "u1", PROCESS_STATE_RUNNING, 10.0f, 100, 1);
    ProcessInfo i2 = create_process_info(2, "p2", "u2", PROCESS_STATE_RUNNING, 20.0f, 200, 2);
    ProcessListItem item1(i1, "Running", nullptr, nullptr);
    ProcessListItem item2(i2, "Running", nullptr, nullptr);
    const ProcessListItem* p1 = &item1;
    const ProcessListItem* p2 = &item2;

    // Descending order
    assert(ProcessListItem::CompareMem(&p1, &p2) > 0);
    assert(ProcessListItem::CompareMem(&p2, &p1) < 0);
    assert(ProcessListItem::CompareMem(&p1, &p1) == 0);
}

void test_compare_threads() {
    ProcessInfo i1 = create_process_info(1, "p1", "u1", PROCESS_STATE_RUNNING, 10.0f, 100, 1);
    ProcessInfo i2 = create_process_info(2, "p2", "u2", PROCESS_STATE_RUNNING, 20.0f, 200, 2);
    ProcessListItem item1(i1, "Running", nullptr, nullptr);
    ProcessListItem item2(i2, "Running", nullptr, nullptr);
    const ProcessListItem* p1 = &item1;
    const ProcessListItem* p2 = &item2;

    // Descending order
    assert(ProcessListItem::CompareThreads(&p1, &p2) > 0);
    assert(ProcessListItem::CompareThreads(&p2, &p1) < 0);
    assert(ProcessListItem::CompareThreads(&p1, &p1) == 0);
}

void test_compare_state() {
    ProcessInfo i1 = create_process_info(1, "p1", "u1", PROCESS_STATE_RUNNING, 10.0f, 100, 1);
    ProcessInfo i2 = create_process_info(2, "p2", "u2", PROCESS_STATE_SLEEPING, 20.0f, 200, 2);
    ProcessListItem item1(i1, "Running", nullptr, nullptr);
    ProcessListItem item2(i2, "Sleeping", nullptr, nullptr);
    const ProcessListItem* p1 = &item1;
    const ProcessListItem* p2 = &item2;

    // Ascending order (RUNNING = 0, SLEEPING = 2)
    assert(ProcessListItem::CompareState(&p1, &p2) < 0);
    assert(ProcessListItem::CompareState(&p2, &p1) > 0);
    assert(ProcessListItem::CompareState(&p1, &p1) == 0);
}

void test_compare_user() {
    ProcessInfo i1 = create_process_info(1, "p1", "alice", PROCESS_STATE_RUNNING, 10.0f, 100, 1);
    ProcessInfo i2 = create_process_info(2, "p2", "BOB", PROCESS_STATE_RUNNING, 20.0f, 200, 2);
    ProcessListItem item1(i1, "Running", nullptr, nullptr);
    ProcessListItem item2(i2, "Running", nullptr, nullptr);
    const ProcessListItem* p1 = &item1;
    const ProcessListItem* p2 = &item2;

    // Case-insensitive ascending
    assert(ProcessListItem::CompareUser(&p1, &p2) < 0);
    assert(ProcessListItem::CompareUser(&p2, &p1) > 0);
    assert(ProcessListItem::CompareUser(&p1, &p1) == 0);
}

int main() {
    std::cout << "Testing ProcessListItem sorting functions..." << std::endl;

    test_compare_cpu();
    test_compare_pid();
    test_compare_name();
    test_compare_mem();
    test_compare_threads();
    test_compare_state();
    test_compare_user();

    std::cout << "All ProcessListItem sorting tests passed!" << std::endl;
    return 0;
}
