#include <cassert>
#include <iostream>
#include <vector>
#include <cmath>

#include "HaikuMocks.h"

// Define extern from HaikuMocks.h
BFont* be_bold_font = nullptr;

// Mock get_cpu_info
static std::vector<cpu_info> gMockCpuInfos;

int get_cpu_info(uint32 first, uint32 count, cpu_info* info) {
    if (gMockCpuInfos.empty()) return B_ERROR;
    if (first + count > gMockCpuInfos.size()) return B_ERROR;
    for (uint32 i = 0; i < count; ++i) {
        info[i] = gMockCpuInfos[first + i];
    }
    return B_OK;
}

#include "mocks/View.h"
// Prevent production ActivityGraphView from being included
#define ACTIVITYGRAPHVIEW_H
#include "mocks/ActivityGraphView.h"

// Redefine private to public for testing
#define private public
#include "../CPUView.cpp"
#include "../Utils.cpp"

void test_initial_call() {
    std::cout << "Testing initial call..." << std::endl;
    CPUView view;
    view.fCpuCount = 2;
    view.fPreviousActiveTime.assign(2, 0);
    view.fCpuInfos.resize(2);
    view.fPerCoreUsage.assign(2, 0.0f);
    view.fPreviousTimeSnapshot = 0;

    gMockCpuInfos.resize(2);
    gMockCpuInfos[0].active_time = 1000;
    gMockCpuInfos[1].active_time = 2000;

    float usage = -1.0f;
    view.GetCPUUsage(5000, usage);

    assert(view.fPreviousTimeSnapshot == 5000);
    assert(view.fPreviousActiveTime[0] == 1000);
    assert(view.fPreviousActiveTime[1] == 2000);
    assert(usage == 0.0f);
    std::cout << "Initial call passed!" << std::endl;
}

void test_normal_calculation() {
    std::cout << "Testing normal calculation..." << std::endl;
    CPUView view;
    view.fCpuCount = 1;
    view.fPreviousActiveTime.assign(1, 1000);
    view.fCpuInfos.resize(1);
    view.fPerCoreUsage.assign(1, 0.0f);
    view.fPreviousTimeSnapshot = 5000;

    gMockCpuInfos.resize(1);
    gMockCpuInfos[0].active_time = 2000; // delta = 1000

    float usage = -1.0f;
    view.GetCPUUsage(10000, usage); // elapsed = 5000

    // usage = 1000 / 5000 * 100 = 20%
    assert(std::abs(usage - 20.0f) < 0.001f);
    assert(view.fPreviousActiveTime[0] == 2000);
    assert(view.fPreviousTimeSnapshot == 10000);
    std::cout << "Normal calculation passed!" << std::endl;
}

void test_time_diff_zero() {
    std::cout << "Testing time difference zero..." << std::endl;
    CPUView view;
    view.fCpuCount = 1;
    view.fPreviousActiveTime.assign(1, 1000);
    view.fCpuInfos.resize(1);
    view.fPreviousTimeSnapshot = 5000;

    float usage = -1.0f;
    view.GetCPUUsage(5000, usage);

    assert(usage == 0.0f);
    assert(view.fPreviousTimeSnapshot == 5000);
    std::cout << "Time difference zero passed!" << std::endl;
}

void test_time_diff_negative() {
    std::cout << "Testing time difference negative..." << std::endl;
    CPUView view;
    view.fCpuCount = 1;
    view.fPreviousActiveTime.assign(1, 1000);
    view.fCpuInfos.resize(1);
    view.fPreviousTimeSnapshot = 5000;

    float usage = -1.0f;
    view.GetCPUUsage(4000, usage);

    assert(usage == 0.0f);
    assert(view.fPreviousTimeSnapshot == 4000);
    std::cout << "Time difference negative passed!" << std::endl;
}

void test_rollover() {
    std::cout << "Testing active time rollover..." << std::endl;
    CPUView view;
    view.fCpuCount = 1;
    view.fPreviousActiveTime.assign(1, 10000);
    view.fCpuInfos.resize(1);
    view.fPreviousTimeSnapshot = 5000;

    gMockCpuInfos.resize(1);
    gMockCpuInfos[0].active_time = 5000; // delta = -5000 -> should be 0

    float usage = -1.0f;
    view.GetCPUUsage(10000, usage);

    assert(usage == 0.0f);
    assert(view.fPreviousActiveTime[0] == 5000);
    std::cout << "Rollover passed!" << std::endl;
}

void test_capping() {
    std::cout << "Testing result capping..." << std::endl;
    CPUView view;
    view.fCpuCount = 1;
    view.fPreviousActiveTime.assign(1, 1000);
    view.fCpuInfos.resize(1);
    view.fPerCoreUsage.assign(1, 0.0f);
    view.fPreviousTimeSnapshot = 5000;

    gMockCpuInfos.resize(1);
    gMockCpuInfos[0].active_time = 10000; // delta = 9000

    float usage = -1.0f;
    view.GetCPUUsage(10000, usage); // elapsed = 5000

    // usage = 9000 / 5000 * 100 = 180% -> should be capped at 100%
    assert(usage == 100.0f);
    assert(view.fPerCoreUsage[0] == 100.0f);
    std::cout << "Capping passed!" << std::endl;
}

void test_cpu_count_zero() {
    std::cout << "Testing CPU count zero..." << std::endl;
    CPUView view;
    view.fCpuCount = 0;

    float usage = -1.0f;
    view.GetCPUUsage(10000, usage);

    assert(usage == -1.0f);
    std::cout << "CPU count zero passed!" << std::endl;
}

int main() {
    if (be_bold_font == nullptr) be_bold_font = new BFont();
    test_initial_call();
    test_normal_calculation();
    test_time_diff_zero();
    test_time_diff_negative();
    test_rollover();
    test_capping();
    test_cpu_count_zero();

    delete be_bold_font;
    be_bold_font = nullptr;

    std::cout << "All CPUView tests passed!" << std::endl;
    return 0;
}
