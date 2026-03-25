#include <cstdio>
#include <cassert>
#include <cstdint>
#include <cmath>

#include <OS.h>

// Including the cpp file directly for testing its implementation
#include "../DataHistory.cpp"

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

int main() {
    printf("Testing DataHistory::ValueAt...\n");
    test_value_at_empty();
    test_value_at_single();
    test_value_at_exact();
    test_value_at_interpolation();
    test_value_at_hint();
    test_value_at_binary_search();
    test_value_at_fast_path();
    test_value_at_duplicate_timestamps();
    printf("All tests passed!\n");
    return 0;
}
