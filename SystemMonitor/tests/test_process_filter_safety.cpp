#include <cassert>
#include <iostream>
#include <cstring>
#include <cstdio>
#include <cinttypes>
#include <cstdint>

#define B_OS_NAME_LENGTH 32
#define B_PRId32 PRId32

typedef int32_t team_id;

enum ProcessState {
    PROCESS_STATE_RUNNING,
    PROCESS_STATE_READY,
    PROCESS_STATE_SLEEPING,
    PROCESS_STATE_UNKNOWN
};

struct ProcessInfo {
    team_id id;
    char name[B_OS_NAME_LENGTH];
    char userName[B_OS_NAME_LENGTH];
    char args[64];
    ProcessState state;
    uint32_t threadCount;
    uint32_t areaCount;
    uint32_t userID;
    uint64_t memoryUsageBytes;
    float cpuUsage;
    int32_t priority;
};

// Mirroring ProcessView::_MatchesFilter logic
bool MatchesFilter(const ProcessInfo& info, const char* searchText) {
    if (searchText == NULL || searchText[0] == '\0')
        return true;

    if (strcasestr(info.name, searchText) != NULL)
        return true;

    if (strcasestr(info.args, searchText) != NULL)
        return true;

    char idStr[32];
    int res = snprintf(idStr, sizeof(idStr), "%" B_PRId32, info.id);
    if (res >= 0 && (size_t)res < sizeof(idStr)) {
        if (strcasestr(idStr, searchText) != NULL)
            return true;
    }

    return false;
}

void test_matches_filter() {
    ProcessInfo info;
    info.id = 12345;
    snprintf(info.name, sizeof(info.name), "test_proc");
    snprintf(info.args, sizeof(info.args), "--arg=val");

    // Test NULL or empty search
    assert(MatchesFilter(info, NULL) == true);
    assert(MatchesFilter(info, "") == true);

    // Test name search
    assert(MatchesFilter(info, "test") == true);
    assert(MatchesFilter(info, "TEST") == true);

    // Test args search
    assert(MatchesFilter(info, "arg") == true);

    // Test PID matching
    assert(MatchesFilter(info, "123") == true);
    assert(MatchesFilter(info, "2345") == true);
    assert(MatchesFilter(info, "999") == false);

    // Test negative ID
    info.id = -500;
    assert(MatchesFilter(info, "-500") == true);

    // Test large ID
    info.id = 2147483647; // INT32_MAX
    assert(MatchesFilter(info, "2147483647") == true);
}

int main() {
    std::cout << "Testing MatchesFilter snprintf safety..." << std::endl;
    test_matches_filter();
    std::cout << "MatchesFilter tests passed successfully!" << std::endl;
    return 0;
}
