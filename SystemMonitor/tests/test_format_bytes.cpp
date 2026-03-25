#include "HaikuMocks.h"
#include <cstdio>
#include <iostream>

// Include the header, implementation will be linked separately
#include "../Utils.h"

static int sFailures = 0;

// Test helper
void test_format(uint64 bytes, int precision, const char* expected) {
    BString str;
    FormatBytes(str, bytes, precision);
    if (!(str == expected)) {
        std::cerr << "FAIL: " << bytes << " bytes (p=" << precision << ") -> expected \""
                  << expected << "\", got \"" << str.String() << "\"" << std::endl;
        sFailures++;
    }
}

void test_format_double(double bytes, int precision, const char* expected) {
    BString str;
    FormatBytes(str, bytes, precision);
    if (!(str == expected)) {
        std::cerr << "FAIL: " << bytes << " bytes (p=" << precision << ") -> expected \""
                  << expected << "\", got \"" << str.String() << "\"" << std::endl;
        sFailures++;
    }
}

int main() {
    printf("Testing FormatBytes logic from Utils.cpp...\n");

    // Bytes range
    test_format(0, 2, "0 B");
    test_format(512, 2, "512 B");
    test_format(1023, 2, "1023 B");
    test_format_double(512.5, 2, "512.5 B");

    // KiB range
    test_format(1024, 2, "1.00 KiB");
    test_format(1024 * 2, 2, "2.00 KiB");
    test_format(1536, 1, "1.5 KiB");
    test_format(1536, 0, "2 KiB"); // Rounding check

    // MiB range
    test_format(1024LL * 1024, 2, "1.00 MiB");
    test_format(1024LL * 1024 * 5, 2, "5.00 MiB");
    test_format(1024LL * 1024 * 1.5, 1, "1.5 MiB");

    // GiB range
    test_format(1024LL * 1024 * 1024, 2, "1.00 GiB");
    test_format(1024LL * 1024 * 1024 * 10, 2, "10.00 GiB");

    // Boundary checks
    test_format(1024 - 1, 2, "1023 B");
    test_format(1024, 2, "1.00 KiB");

    test_format(1024LL * 1024 - 1, 2, "1024.00 KiB"); // 1023.999...
    test_format(1024LL * 1024, 2, "1.00 MiB");

    test_format(1024LL * 1024 * 1024 - 1024, 2, "1024.00 MiB"); // 1023.999...
    test_format(1024LL * 1024 * 1024, 2, "1.00 GiB");

    // Double overload tests
    test_format_double(1024.0, 2, "1.00 KiB");
    test_format_double(1024.0 * 1024.0 * 1024.0 * 1.25, 3, "1.250 GiB");

    if (sFailures > 0) {
        printf("FAILED: %d test cases failed!\n", sFailures);
        return 1;
    }

    printf("SUCCESS: All FormatBytes tests passed!\n");
    return 0;
}
