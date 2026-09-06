#include <cstdio>
#include <cassert>
#include <cstdint>
#include <cmath>
#include <iostream>
#include <vector>

#include <OS.h>

// Including the cpp file directly for testing its implementation
#include "../DataHistory.cpp"

void test_constructor() {
    printf("Testing Constructor...\n");
    // memorize = 10000000 (10s), interval = 100000 (0.1s)
    // Should result in buffer size of 100
    DataHistory history(10000000, 100000);
    assert(history.MaximumValue() == 0);
    assert(history.MinimumValue() == 0);
    assert(history.Start() == 0);
    assert(history.End() == 0);
}

void test_add_value_and_min_max() {
    printf("Testing AddValue and Min/Max...\n");
    DataHistory history(500000, 100000); // size = 5

    history.AddValue(100000, 10);
    assert(history.MinimumValue() == 10);
    assert(history.MaximumValue() == 10);
    assert(history.Start() == 100000);
    assert(history.End() == 100000);

    history.AddValue(200000, 50);
    assert(history.MinimumValue() == 10);
    assert(history.MaximumValue() == 50);
    assert(history.Start() == 100000);
    assert(history.End() == 200000);

    history.AddValue(300000, 5);
    assert(history.MinimumValue() == 5);
    assert(history.MaximumValue() == 50);
    assert(history.End() == 300000);

    history.AddValue(400000, 20);
    history.AddValue(500000, 15);
    assert(history.MinimumValue() == 5);
    assert(history.MaximumValue() == 50);

    // Add a 6th value, which should evict the oldest (value 10 at 100000)
    history.AddValue(600000, 30);
    assert(history.MinimumValue() == 5);
    // The max was 50 (at 200000), so it should still be 50.
    assert(history.MaximumValue() == 50);
    assert(history.Start() == 200000);
    assert(history.End() == 600000);

    // Add a 7th value, which evicts the 2nd value (value 50 at 200000)
    // Now the max should drop since 50 is gone. The remaining values are 5, 20, 15, 30, and the new one.
    history.AddValue(700000, 25);
    assert(history.MaximumValue() == 30); // Max is now 30
    assert(history.MinimumValue() == 5);  // Min is still 5
}

void test_value_at() {
    printf("Testing ValueAt...\n");
    DataHistory history(1000000, 100000); // size 10

    history.AddValue(100000, 10);
    history.AddValue(200000, 20);
    history.AddValue(300000, 30);

    int32 hint = -1;
    // Exact match
    assert(history.ValueAt(100000, &hint) == 10);

    // Interpolation (middle of interval)
    assert(history.ValueAt(150000, &hint) == 15);

    // Exact match on next interval
    assert(history.ValueAt(200000, &hint) == 20);

    // Past the end of available data (should clamp to last value or next value logic)
    // ValueAt implementation: if time > last item's time, and there is no nextItem, it returns item->value
    assert(history.ValueAt(400000, &hint) == 30);

    // Before the start of available data
    // If we request a time before the first item, ValueAt returns 0 because left=0, and left > right or it loops.
    // Wait, let's verify ValueAt for time < first item.
    // ValueAt: if item->time > time, right = index - 1. Eventually left > right and loop terminates, returning 0.
    assert(history.ValueAt(50000, &hint) == 0);
}

void test_set_refresh_interval() {
    printf("Testing SetRefreshInterval...\n");
    DataHistory history(1000000, 100000); // size 10

    for (int i = 1; i <= 10; i++) {
        history.AddValue(i * 100000, i * 10);
    }

    assert(history.MaximumValue() == 100);
    assert(history.MinimumValue() == 10);

    // Shrink the duration logic: SetRefreshInterval changes the interval, but calculates
    // new size based on the old duration: duration = fBuffer.Size() * fRefreshInterval.
    // Old duration = 10 * 100000 = 1000000.
    // New interval = 200000 -> new size = 1000000 / 200000 = 5.
    // However, the minimum size is hardcoded to 10 in SetRefreshInterval.
    // So new size = max(1000000 / 200000, 10) = max(5, 10) = 10.
    // The buffer will still have size 10, so it keeps all 10 items.
    history.SetRefreshInterval(200000);

    assert(history.MaximumValue() == 100);
    assert(history.MinimumValue() == 10);

    // Add a new value, eviction should occur, replacing the oldest (10)
    history.AddValue(1100000, 50); // items: 20..100, 50
    assert(history.MinimumValue() == 20);
    assert(history.MaximumValue() == 100);
}

void test_value_at_empty() {
    DataHistory history(1000, 10);
    assert(history.ValueAt(50) == 0);
    printf("test_value_at_empty passed\n");
}

void test_value_at_single() {
    DataHistory history(1000, 10);
    history.AddValue(100, 500);
    assert(history.ValueAt(50) == 0); // before the item -> 0
    assert(history.ValueAt(100) == 500);
    assert(history.ValueAt(150) == 500); // after the item
    printf("test_value_at_single passed\n");
}

void test_value_at_exact() {
    DataHistory history(1000, 10);
    history.AddValue(100, 500);
    history.AddValue(200, 600);
    history.AddValue(300, 700);

    assert(history.ValueAt(100) == 500);
    assert(history.ValueAt(200) == 600);
    assert(history.ValueAt(300) == 700);
    printf("test_value_at_exact passed\n");
}

void test_value_at_interpolation() {
    DataHistory history(1000, 10);
    history.AddValue(100, 500);
    history.AddValue(200, 700);

    // halfway
    assert(history.ValueAt(150) == 600);

    // 25%
    assert(history.ValueAt(125) == 550);

    // out of bounds
    assert(history.ValueAt(50) == 0);
    assert(history.ValueAt(250) == 700);
    printf("test_value_at_interpolation passed\n");
}

void test_value_at_hint() {
    DataHistory history(1000, 10);
    history.AddValue(100, 500);
    history.AddValue(200, 600);
    history.AddValue(300, 700);
    history.AddValue(400, 800);

    int32 hint = -1;
    assert(history.ValueAt(250, &hint) == 650);
    assert(hint == 1);

    assert(history.ValueAt(350, &hint) == 750);
    assert(hint == 2);
    printf("test_value_at_hint passed\n");
}

void test_value_at_binary_search() {
    DataHistory history(10000, 10);
    for (int i = 0; i < 100; i++) {
        history.AddValue(i * 10, i * 100);
    }

    int32 hint = -1;
    assert(history.ValueAt(255, &hint) == 2550);
    assert(hint == 25);

    assert(history.ValueAt(15, nullptr) == 150);
    printf("test_value_at_binary_search passed\n");
}

void test_value_at_fast_path() {
    DataHistory history(10000, 10);
    for (int i = 0; i < 10; i++) {
        history.AddValue(i * 100, i * 1000);
    }

    int32 hint = 0;
    // Sequential queries will trigger the fast path
    assert(history.ValueAt(150, &hint) == 1500);
    assert(hint == 1);

    // Within the same interval
    assert(history.ValueAt(160, &hint) == 1600);
    assert(hint == 1);

    // Very next interval
    assert(history.ValueAt(250, &hint) == 2500);
    assert(hint == 2);

    printf("test_value_at_fast_path passed\n");
}

void test_value_at_duplicate_timestamps() {
    DataHistory history(1000, 10);
    history.AddValue(100, 500);
    history.AddValue(100, 700);

    // With duplicate timestamps, interpolation shouldn't divide by zero
    int64 val = history.ValueAt(100);
    assert(val == 500 || val == 700);
    printf("test_value_at_duplicate_timestamps passed\n");
}

void test_value_at_stale_hint() {
    DataHistory history(1000, 10);
    history.AddValue(100, 500);
    history.AddValue(200, 600);

    int32 staleHint = 50; // Stale hint index out of bounds
    assert(history.ValueAt(150, &staleHint) == 550);
    printf("test_value_at_stale_hint passed\n");
}

int main() {
    printf("Starting DataHistory tests...\n");
    test_constructor();
    test_add_value_and_min_max();
    test_value_at();
    test_set_refresh_interval();
    printf("Testing DataHistory::ValueAt...\n");
    test_value_at_empty();
    test_value_at_single();
    test_value_at_exact();
    test_value_at_interpolation();
    test_value_at_hint();
    test_value_at_binary_search();
    test_value_at_fast_path();
    test_value_at_duplicate_timestamps();
    test_value_at_stale_hint();
    printf("All DataHistory tests passed!\n");
    return 0;
}
