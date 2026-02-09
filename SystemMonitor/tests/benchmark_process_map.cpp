#include <iostream>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <random>

// Mock ProcessListItem
struct ProcessListItem {
    int id;
    int data;
    ProcessListItem(int i) : id(i), data(0) {}
    void Update(int d) { data = d; }
};

using team_id = int;
using namespace std;

int main() {
    const int num_procs = 2000;
    const int iterations = 100000;

    vector<int> team_ids;
    for (int i = 0; i < num_procs; ++i) {
        team_ids.push_back(i);
    }

    long long baseline_duration = 0;
    long long optimized_duration = 0;

    // Baseline: current code logic
    {
        unordered_map<team_id, ProcessListItem*> fTeamItemMap;
        // Pre-fill
        for (int id : team_ids) {
            fTeamItemMap[id] = new ProcessListItem(id);
        }

        auto start = chrono::high_resolution_clock::now();

        for (int k = 0; k < iterations; ++k) {
            for (int id : team_ids) {
                ProcessListItem* item;
                // The logic from ProcessView.cpp:
                if (fTeamItemMap.find(id) == fTeamItemMap.end()) {
                    item = new ProcessListItem(id);
                    fTeamItemMap[id] = item;
                } else {
                    item = fTeamItemMap[id]; // Second lookup
                    item->Update(k);
                }
            }
        }

        auto end = chrono::high_resolution_clock::now();
        baseline_duration = chrono::duration_cast<chrono::microseconds>(end - start).count();

        for (auto& pair : fTeamItemMap) delete pair.second;
    }

    // Optimized
    {
        unordered_map<team_id, ProcessListItem*> fTeamItemMap;
        // Pre-fill
        for (int id : team_ids) {
            fTeamItemMap[id] = new ProcessListItem(id);
        }

        auto start = chrono::high_resolution_clock::now();

        for (int k = 0; k < iterations; ++k) {
            for (int id : team_ids) {
                ProcessListItem* item;
                // Optimized logic
                auto it = fTeamItemMap.find(id);
                if (it == fTeamItemMap.end()) {
                    item = new ProcessListItem(id);
                    fTeamItemMap.emplace(id, item);
                } else {
                    item = it->second; // No lookup
                    item->Update(k);
                }
            }
        }

        auto end = chrono::high_resolution_clock::now();
        optimized_duration = chrono::duration_cast<chrono::microseconds>(end - start).count();

        for (auto& pair : fTeamItemMap) delete pair.second;
    }

    cout << "Baseline: " << baseline_duration << " us" << endl;
    cout << "Optimized: " << optimized_duration << " us" << endl;
    double improvement = 100.0 * (double)(baseline_duration - optimized_duration) / baseline_duration;
    cout << "Improvement: " << improvement << "%" << endl;

    return 0;
}
