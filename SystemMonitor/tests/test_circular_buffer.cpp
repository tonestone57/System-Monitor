#include <cstdio>
#include <cassert>
#include <string.h>
#include <cstdint>

// Mock OS.h functionality for standalone testing if not available
#ifndef _OS_H
#ifndef B_OK
#define B_OK 0
#endif
#ifndef B_NO_MEMORY
#define B_NO_MEMORY -1
#endif
typedef int32_t status_t;
typedef int32_t int32;
typedef uint32_t uint32;
typedef long long bigtime_t;
#endif

#include "../CircularBuffer.h"

int main() {
    printf("Testing CircularBuffer...\n");

    // Test Constructor
    CircularBuffer<int> buffer(5);
    assert(buffer.InitCheck() == B_OK);
    assert(buffer.Size() == 5);
    assert(buffer.CountItems() == 0);
    assert(buffer.IsEmpty());

    // Test AddItem
    for (int i = 0; i < 5; i++) {
        buffer.AddItem(i);
    }
    assert(buffer.CountItems() == 5);
    assert(*buffer.ItemAt(0) == 0);
    assert(*buffer.ItemAt(4) == 4);

    // Test Overwrite
    buffer.AddItem(5);
    assert(buffer.CountItems() == 5);
    assert(*buffer.ItemAt(0) == 1); // Oldest should be 1 now
    assert(*buffer.ItemAt(4) == 5); // Newest should be 5

    // Test Copy Constructor
    CircularBuffer<int> buffer2(buffer);
    assert(buffer2.Size() == 5);
    assert(buffer2.CountItems() == 5);
    assert(*buffer2.ItemAt(0) == 1);
    assert(*buffer2.ItemAt(4) == 5);

    // Test Assignment
    CircularBuffer<int> buffer3(10);
    buffer3 = buffer;
    assert(buffer3.Size() == 5);
    assert(buffer3.CountItems() == 5);
    assert(*buffer3.ItemAt(0) == 1);
    assert(*buffer3.ItemAt(4) == 5);

    // Test SetSize (Grow)
    buffer.SetSize(10);
    assert(buffer.Size() == 10);
    assert(buffer.CountItems() == 5);
    assert(*buffer.ItemAt(0) == 1);
    assert(*buffer.ItemAt(4) == 5);

    // Add more items
    for (int i = 6; i < 11; i++) {
        buffer.AddItem(i);
    }
    assert(buffer.CountItems() == 10);
    assert(*buffer.ItemAt(0) == 1);
    assert(*buffer.ItemAt(9) == 10);

    // Test SetSize (Shrink)
    buffer.SetSize(3);
    assert(buffer.Size() == 3);
    assert(buffer.CountItems() == 3);
    // Should keep newest 3: 8, 9, 10
    assert(*buffer.ItemAt(0) == 8);
    assert(*buffer.ItemAt(2) == 10);

    // Test SetSize (Zero)
    buffer.SetSize(0);
    assert(buffer.Size() == 0);
    assert(buffer.CountItems() == 0);
    assert(buffer.ItemAt(0) == NULL);

    // Test MakeEmpty
    buffer.MakeEmpty();
    assert(buffer.IsEmpty());
    assert(buffer.CountItems() == 0);

    printf("All tests passed!\n");
    return 0;
}
