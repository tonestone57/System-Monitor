#include <vector>
#include <iostream>
#include <chrono>

struct BPoint { float x, y; };

int main() {
    const int iterations = 10000;
    const int max_size = 2000; // Simulate 1080p screen width roughly

    // Baseline: No reserve
    {
        auto start = std::chrono::high_resolution_clock::now();
        for (int k = 0; k < iterations; ++k) {
            std::vector<BPoint> v;
            // Simulate dragging from size 100 to max_size
            for (int i = 100; i <= max_size; ++i) {
                v.resize(i);
                // Access data to ensure it's used
                v[0].x = (float)i;
            }
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << "Baseline duration: "
                  << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()
                  << " us" << std::endl;
    }

    // Optimized: Reserve in constructor (simulated)
    {
        auto start = std::chrono::high_resolution_clock::now();
        for (int k = 0; k < iterations; ++k) {
            std::vector<BPoint> v;
            v.reserve(max_size + 64); // Simulate constructor reserve

            // Simulate dragging from size 100 to max_size
            for (int i = 100; i <= max_size; ++i) {
                v.resize(i);
                v[0].x = (float)i;
            }
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << "Optimized duration: "
                  << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()
                  << " us" << std::endl;
    }

    return 0;
}
