#include <iostream>
#include <vector>
#include <unordered_set>
#include <chrono>

struct ProcessListItem {
    int id;
    bool isVisible;

    ProcessListItem(int i) : id(i), isVisible(false) {}
};

// Mock BListView
class BListView {
public:
    std::vector<ProcessListItem*> items;

    void AddItem(ProcessListItem* item) {
        items.push_back(item);
    }

    void RemoveItem(ProcessListItem* item) {
        for (auto it = items.begin(); it != items.end(); ++it) {
            if (*it == item) {
                items.erase(it);
                return;
            }
        }
    }
};

void run_baseline(int iterations, int num_items) {
    BListView list_view;
    std::unordered_set<ProcessListItem*> fVisibleItems;
    std::vector<ProcessListItem*> all_items;

    for (int i = 0; i < num_items; ++i) {
        all_items.push_back(new ProcessListItem(i));
        if (i % 2 == 0) {
            list_view.AddItem(all_items.back());
            fVisibleItems.insert(all_items.back());
        }
    }

    auto start = std::chrono::high_resolution_clock::now();

    for (int iter = 0; iter < iterations; ++iter) {
        for (int i = 0; i < num_items; ++i) {
            ProcessListItem* item = all_items[i];
            bool match = (i + iter) % 2 == 0; // Toggle match state

            if (match) {
                if (fVisibleItems.insert(item).second)
                    list_view.AddItem(item);
            } else {
                if (fVisibleItems.erase(item) > 0)
                    list_view.RemoveItem(item);
            }
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "Baseline duration: " << duration.count() << " ms\n";

    for (auto item : all_items) {
        delete item;
    }
}

void run_optimized(int iterations, int num_items) {
    BListView list_view;
    std::vector<ProcessListItem*> all_items;

    for (int i = 0; i < num_items; ++i) {
        all_items.push_back(new ProcessListItem(i));
        if (i % 2 == 0) {
            list_view.AddItem(all_items.back());
            all_items.back()->isVisible = true;
        }
    }

    auto start = std::chrono::high_resolution_clock::now();

    for (int iter = 0; iter < iterations; ++iter) {
        for (int i = 0; i < num_items; ++i) {
            ProcessListItem* item = all_items[i];
            bool match = (i + iter) % 2 == 0; // Toggle match state

            if (match) {
                if (!item->isVisible) {
                    item->isVisible = true;
                    list_view.AddItem(item);
                }
            } else {
                if (item->isVisible) {
                    item->isVisible = false;
                    list_view.RemoveItem(item);
                }
            }
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "Optimized duration: " << duration.count() << " ms\n";

    for (auto item : all_items) {
        delete item;
    }
}

int main() {
    int iterations = 1000;
    int num_items = 1000;

    std::cout << "Benchmarking visibility updates with " << num_items << " items and " << iterations << " iterations\n";

    run_baseline(iterations, num_items);
    run_optimized(iterations, num_items);

    return 0;
}
