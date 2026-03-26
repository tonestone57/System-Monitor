#include <cassert>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

#include "HaikuMocks.h"
#include "../Utils.h"

BFont* be_bold_font = nullptr;

void test_get_core_count() {
    // GetCoreCount uses a static IIFE lambda to cache the result, so we must test it in separate processes
    pid_t pid = fork();
    if (pid == 0) {
        MockCpuCount() = 4;
        MockSystemInfoResult() = B_OK;
        assert(GetCoreCount() == 4);

        // Reset global mock states (even though process exits, it's good practice)
        MockCpuCount() = 1;
        MockSystemInfoResult() = B_OK;
        exit(0);
    } else {
        int status;
        waitpid(pid, &status, 0);
        assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    }

    pid = fork();
    if (pid == 0) {
        MockSystemInfoResult() = -1;
        assert(GetCoreCount() == 1);

        // Reset global mock states
        MockCpuCount() = 1;
        MockSystemInfoResult() = B_OK;
        exit(0);
    } else {
        int status;
        waitpid(pid, &status, 0);
        assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    }
}

int main() {
    std::cout << "Testing Utils.cpp..." << std::endl;

    // Test FormatBytes (uint64)
    BString out;
    FormatBytes(out, (uint64)0);
    assert(out == "0 B");

    FormatBytes(out, (uint64)500);
    assert(out == "500 B");

    FormatBytes(out, (uint64)1024);
    assert(out == "1.00 KiB");

    FormatBytes(out, (uint64)(1024 * 1.5));
    assert(out == "1.50 KiB");

    FormatBytes(out, (uint64)(1024 * 1024));
    assert(out == "1.00 MiB");

    FormatBytes(out, (uint64)(1024 * 1024 * 1024));
    assert(out == "1.00 GiB");

    FormatBytes(out, (uint64)(1024ULL * 1024 * 1024 * 1024));
    assert(out == "1024.00 GiB" || out == "1.00 TiB");

    // Test FormatBytes (double)
    FormatBytes(out, 1.5 * 1024);
    assert(out == "1.50 KiB");

    // Test precision
    FormatBytes(out, 1.12345 * 1024, 0);
    assert(out == "1 KiB");

    FormatBytes(out, 1.12345 * 1024, 1);
    assert(out == "1.1 KiB");

    // Test BytesToMiB
    assert(BytesToMiB(0) == 0);
    assert(BytesToMiB(1) == 1);

    // Test 1 MiB boundary (1048576 bytes)
    assert(BytesToMiB(1048575) == 1); // 1 byte below exactly 1 MiB
    assert(BytesToMiB(1048576) == 1); // Exactly 1 MiB
    assert(BytesToMiB(1048577) == 2); // 1 byte above exactly 1 MiB

    // Test 2 MiB boundary (2097152 bytes)
    assert(BytesToMiB(1024 * 1024 * 2 - 1) == 2); // 1 byte below exactly 2 MiB
    assert(BytesToMiB(1024 * 1024 * 2) == 2);     // Exactly 2 MiB
    assert(BytesToMiB(1024 * 1024 * 2 + 1) == 3); // 1 byte above exactly 2 MiB

    // Test GetScaleFactor
    BFont font;
    assert(GetScaleFactor(&font) == 1.0f); // default mocked size is 12.0f

    // Test FormatHertz
    assert(FormatHertz(0) == "0 Hz");
    assert(FormatHertz(1000) == "1 kHz");
    assert(FormatHertz(1000000) == "1 MHz");
    assert(FormatHertz(1000000000) == "1.00 GHz");
    assert(FormatHertz(1500) == "2 kHz");
    assert(FormatHertz(1500000) == "2 MHz");
    assert(FormatHertz(1500000000) == "1.50 GHz");

    // Test FormatUptime
    assert(FormatUptime(1000000) == "mock_duration");

    // Test FormatSpeed
    assert(FormatSpeed(0, 1000000) == "0 B/s");
    assert(FormatSpeed(1024, 1000000) == "1.00 KiB/s");
    assert(FormatSpeed(1024, 500000) == "2.00 KiB/s");

    test_get_core_count();

    std::cout << "All Utils tests passed!" << std::endl;
    return 0;
}
