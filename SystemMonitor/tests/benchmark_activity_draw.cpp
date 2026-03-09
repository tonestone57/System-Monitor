#include <vector>
#include <iostream>
#include <chrono>

struct BPoint { float x, y; };

void simulate_draw(std::vector<BPoint>& v, int size, bool optimized) {
    if (optimized) {
        if (v.size() < (size_t)size)
            v.resize(size);
    } else {
        v.resize(size);
    }
    // Access some data to prevent optimization
    if (size > 0) {
        v[0].x = (float)size;
        v[size-1].y = (float)size;
    }
}

int main() {
    const int iterations = 100000;
    const int full_size = 2000;
    const int partial_size = 10;

    // Baseline: Unconditional resize
    {
        std::vector<BPoint> v;
        v.reserve(full_size + 64);
        auto start = std::chrono::high_resolution_clock::now();
        for (int k = 0; k < iterations; ++k) {
            // Simulate 1 full redraw followed by 10 partial updates
            if (k % 11 == 0) {
                simulate_draw(v, full_size, false);
            } else {
                simulate_draw(v, partial_size, false);
            }
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << "Baseline duration: "
                  << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()
                  << " us" << std::endl;
    }

    // Optimized: Only resize up
    {
        std::vector<BPoint> v;
        v.reserve(full_size + 64);
        auto start = std::chrono::high_resolution_clock::now();
        for (int k = 0; k < iterations; ++k) {
            if (k % 11 == 0) {
                simulate_draw(v, full_size, true);
            } else {
                simulate_draw(v, partial_size, true);
            }
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << "Optimized duration: "
                  << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()
                  << " us" << std::endl;
    }

    return 0;
}
