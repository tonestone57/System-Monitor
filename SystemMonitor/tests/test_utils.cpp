#include <cassert>
#include <iostream>
#include <string>

#include "HaikuMocks.h"
#include "../Utils.h"

BFont* be_bold_font = nullptr;

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
    assert(BytesToMiB(1048576) == 1);
    assert(BytesToMiB(1048577) == 2);
    assert(BytesToMiB(1024 * 1024 * 2) == 2);

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

    std::cout << "All Utils tests passed!" << std::endl;
    return 0;
}
