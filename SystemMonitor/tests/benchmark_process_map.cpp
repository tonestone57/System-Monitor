#include <iostream>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <utility>

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
    const int num_procs = 10000;
    const int iterations = 1000;

    vector<int> team_ids;
    for (int i = 0; i < num_procs; ++i) {
        team_ids.push_back(i);
    }

    long long current_duration = 0;
    long long single_lookup_duration = 0;

    // Current (Baseline): find + emplace (2 lookups for insertion)
    {
        auto start = chrono::high_resolution_clock::now();
        for (int k = 0; k < iterations; ++k) {
            unordered_map<team_id, ProcessListItem*> map;

            for (int id : team_ids) {
                auto it = map.find(id);
                if (it == map.end()) {
                    ProcessListItem* item = new ProcessListItem(id);
                    map.emplace(id, item);
                } else {
                    it->second->Update(k);
                }
            }

            for (auto& p : map) delete p.second;
        }
        auto end = chrono::high_resolution_clock::now();
        current_duration = chrono::duration_cast<chrono::microseconds>(end - start).count();
    }

    // Proposed: emplace + check (1 lookup always)
    {
        auto start = chrono::high_resolution_clock::now();
        for (int k = 0; k < iterations; ++k) {
            unordered_map<team_id, ProcessListItem*> map;

            for (int id : team_ids) {
                auto result = map.emplace(id, nullptr);
                if (result.second) {
                    result.first->second = new ProcessListItem(id);
                } else {
                    result.first->second->Update(k);
                }
            }

            for (auto& p : map) delete p.second;
        }
        auto end = chrono::high_resolution_clock::now();
        single_lookup_duration = chrono::duration_cast<chrono::microseconds>(end - start).count();
    }

    cout << "Current (find+emplace): " << current_duration << " us" << endl;
    cout << "Proposed (emplace check): " << single_lookup_duration << " us" << endl;

    double improvement = 100.0 * (double)(current_duration - single_lookup_duration) / current_duration;
    cout << "Improvement: " << improvement << "%" << endl;

    return 0;
}
