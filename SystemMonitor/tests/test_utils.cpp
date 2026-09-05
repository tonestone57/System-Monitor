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

void test_get_memory_usage() {
    uint64 used = 0, total = 0, physical = 0;

    // Test when get_system_info fails
    MockSystemInfoResult() = B_ERROR;
    GetMemoryUsage(used, total, physical);
    assert(used == 0);
    assert(total == 0);
    assert(physical == 0);

    // Test when get_system_info succeeds
    MockSystemInfoResult() = B_OK;
    MockMaxPages() = 1000;
    MockUsedPages() = 500;
    MockIgnoredPages() = 100;

    GetMemoryUsage(used, total, physical);
    assert(total == 1000 * B_PAGE_SIZE);
    assert(used == 500 * B_PAGE_SIZE);
    assert(physical == (1000 + 100) * B_PAGE_SIZE);

    // Reset global mock states
    MockSystemInfoResult() = B_OK;
    MockMaxPages() = 0;
    MockUsedPages() = 0;
    MockIgnoredPages() = 0;
}

void test_get_cached_memory_bytes() {
    system_info info;

    // Test with zeros
    info.cached_pages = 0;
    info.block_cache_pages = 0;
    assert(GetCachedMemoryBytes(info) == 0);

    // Test with some values
    info.cached_pages = 100;
    info.block_cache_pages = 200;
    // (100 + 200) * 4096 = 300 * 4096 = 1228800
    assert(GetCachedMemoryBytes(info) == 300ULL * B_PAGE_SIZE);
    assert(GetCachedMemoryBytes(info) == 1228800ULL);

    // Test with large values
    info.cached_pages = 1000000;
    info.block_cache_pages = 1000000;
    assert(GetCachedMemoryBytes(info) == 2000000ULL * B_PAGE_SIZE);
}

void test_get_locale() {
    // Test default case (both LC_ALL and LANG unset)
    pid_t pid = fork();
    if (pid == 0) {
        unsetenv("LC_ALL");
        unsetenv("LANG");
        assert(GetLocale() == "en_US.UTF-8");
        exit(0);
    } else {
        int status;
        waitpid(pid, &status, 0);
        assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    }

    // Test LC_ALL set
    pid = fork();
    if (pid == 0) {
        setenv("LC_ALL", "fr_FR.UTF-8", 1);
        unsetenv("LANG");
        assert(GetLocale() == "fr_FR.UTF-8");
        exit(0);
    } else {
        int status;
        waitpid(pid, &status, 0);
        assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    }

    // Test LANG set, LC_ALL unset
    pid = fork();
    if (pid == 0) {
        unsetenv("LC_ALL");
        setenv("LANG", "de_DE.UTF-8", 1);
        assert(GetLocale() == "de_DE.UTF-8");
        exit(0);
    } else {
        int status;
        waitpid(pid, &status, 0);
        assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    }

    // Test LC_ALL takes precedence
    pid = fork();
    if (pid == 0) {
        setenv("LC_ALL", "es_ES.UTF-8", 1);
        setenv("LANG", "de_DE.UTF-8", 1);
        assert(GetLocale() == "es_ES.UTF-8");
        exit(0);
    } else {
        int status;
        waitpid(pid, &status, 0);
        assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    }

    // Test empty strings (should fallback to default or LANG)
    pid = fork();
    if (pid == 0) {
        setenv("LC_ALL", "", 1);
        setenv("LANG", "it_IT.UTF-8", 1);
        // Expecting LANG if LC_ALL is empty
        assert(GetLocale() == "it_IT.UTF-8");
        exit(0);
    } else {
        int status;
        waitpid(pid, &status, 0);
        assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    }

    // Test excessively long string
    pid = fork();
    if (pid == 0) {
        std::string longStr(1024, 'A');
        setenv("LC_ALL", longStr.c_str(), 1);
        BString locale = GetLocale();
        assert(locale.Length() < 1024);
        assert(locale.Length() > 0);
        exit(0);
    } else {
        int status;
        waitpid(pid, &status, 0);
        assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    }

    // Test control characters / newlines
    pid = fork();
    if (pid == 0) {
        setenv("LC_ALL", "en_US.UTF-8\nInjected: True", 1);
        BString locale = GetLocale();
        assert(locale.FindFirst("\n") == -1);
        assert(locale.FindFirst("\r") == -1);
        exit(0);
    } else {
        int status;
        waitpid(pid, &status, 0);
        assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    }
}

void test_get_battery_capacity() {
    // Calling GetBatteryCapacity() in non-battery / mock Linux environment returns "Unknown" or valid string
    BString capacity = GetBatteryCapacity();
    assert(!capacity.IsEmpty());
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
    test_get_locale();
    test_get_memory_usage();
    test_get_cached_memory_bytes();
    test_get_battery_capacity();

    std::cout << "All Utils tests passed!" << std::endl;
    return 0;
}
