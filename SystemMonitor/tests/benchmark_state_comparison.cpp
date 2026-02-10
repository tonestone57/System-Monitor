#include <iostream>
#include <chrono>
#include <cstring>
#include <vector>
#include <random>

// --- Mocks ---

// Mock BString to simulate string comparison
class BString {
public:
    BString() : fStr(nullptr) {}
    BString(const char* str) { SetTo(str); }
    ~BString() { if (fStr) free(fStr); }

    void SetTo(const char* str) {
        if (fStr) free(fStr);
        if (str) fStr = strdup(str);
        else fStr = strdup("");
    }

    const char* String() const { return fStr ? fStr : ""; }

    bool operator!=(const char* other) const {
        return strcmp(String(), other) != 0;
    }

    bool operator==(const char* other) const {
        return strcmp(String(), other) == 0;
    }

    // Assignment for benchmark
    BString& operator=(const char* str) {
        SetTo(str);
        return *this;
    }

private:
    char* fStr;
};

// Enum from ProcessView.h
enum ProcessState {
    PROCESS_STATE_RUNNING,
    PROCESS_STATE_READY,
    PROCESS_STATE_SLEEPING,
    PROCESS_STATE_UNKNOWN
};

// Struct from ProcessView.h (simplified for benchmark)
struct ProcessInfo {
    ProcessState state;
    // other fields ignored
};

// Mock ProcessListItem
class ProcessListItem {
public:
    ProcessListItem(ProcessState state, const char* stateStr) {
        fInfo.state = state;
        fCachedState = stateStr;
    }

    // Current implementation: String comparison
    void UpdateString(const ProcessInfo& info, const char* stateStr, bool force = false) {
        bool stateChanged = force || fCachedState != stateStr;

        fInfo = info;

        if (stateChanged)
            fCachedState = stateStr;
    }

    // Optimized implementation: Enum comparison
    void UpdateEnum(const ProcessInfo& info, const char* stateStr, bool force = false) {
        bool stateChanged = force || fInfo.state != info.state;

        fInfo = info;

        if (stateChanged)
            fCachedState = stateStr;
    }

private:
    ProcessInfo fInfo;
    BString fCachedState;
};

// --- Benchmark ---

int main() {
    const int iterations = 10000000;
    std::vector<ProcessInfo> testData;
    std::vector<const char*> stateStrings;

    // Setup test data
    stateStrings.push_back("Running");
    stateStrings.push_back("Ready");
    stateStrings.push_back("Sleeping");
    stateStrings.push_back("Unknown");

    std::srand(0);
    for (int i = 0; i < iterations; ++i) {
        ProcessInfo info;
        // 90% chance state doesn't change, 10% change
        int r = std::rand() % 100;
        int stateIdx = (r < 90) ? 0 : (std::rand() % 4);

        info.state = (ProcessState)stateIdx;
        testData.push_back(info);
    }

    // Baseline Benchmark (String Comparison)
    {
        ProcessListItem item(PROCESS_STATE_RUNNING, "Running");
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < iterations; ++i) {
            const char* str = stateStrings[(int)testData[i].state];
            item.UpdateString(testData[i], str);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        std::cout << "Baseline (String Comparison): " << duration << " us" << std::endl;
    }

    // Optimized Benchmark (Enum Comparison)
    {
        ProcessListItem item(PROCESS_STATE_RUNNING, "Running");
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < iterations; ++i) {
            const char* str = stateStrings[(int)testData[i].state];
            item.UpdateEnum(testData[i], str);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        std::cout << "Optimized (Enum Comparison): " << duration << " us" << std::endl;
    }

    return 0;
}
