#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <chrono>
#include <cstring>
#include <stdio.h>

// Mock classes to simulate Haiku API behavior relevant to the benchmark

class BStringField {
public:
    // Simulates allocation and copy of string
    BStringField(const char* str) : fString(str ? str : "") {}
    virtual ~BStringField() {}

    // Simulates setting string (reusing object)
    void SetString(const char* str) { fString = (str ? str : ""); }

    const char* String() const { return fString.c_str(); }
private:
    std::string fString;
};

class BRow {
public:
    BRow() {
        memset(fFields, 0, sizeof(fFields));
    }
    virtual ~BRow() {
        for (int i = 0; i < 7; ++i) if (fFields[i]) delete fFields[i];
    }
    // Takes ownership of field
    void SetField(BStringField* field, int index) {
        if (fFields[index]) delete fFields[index];
        fFields[index] = field;
    }
    BStringField* GetField(int index) {
        return fFields[index];
    }
private:
    BStringField* fFields[7];
};

// Simulation parameters
const int kNumInterfaces = 20; // More interfaces makes it more visible
const int kNumIterations = 20000;

void BenchmarkBaseline() {
    std::map<std::string, BRow*> rowMap;
    // Setup initial rows
    for (int i = 0; i < kNumInterfaces; ++i) {
        std::string name = "eth" + std::to_string(i);
        rowMap[name] = new BRow();
    }

    auto start = std::chrono::high_resolution_clock::now();

    for (int iter = 0; iter < kNumIterations; ++iter) {
        for (int i = 0; i < kNumInterfaces; ++i) {
            std::string name = "eth" + std::to_string(i);

            // Simulate values
            std::string type = "Ethernet";
            std::string address = "192.168.1." + std::to_string(i);
            std::string sent = std::to_string(iter * 100) + " B";
            std::string recv = std::to_string(iter * 200) + " B";
            std::string tx = "10 KB/s";
            std::string rx = "20 KB/s";

            BRow* row = rowMap[name];
            // Baseline: Always new BStringField
            // This mirrors:
            // row->SetField(new BStringField(typeStr), kInterfaceTypeColumn);
            // ...
            row->SetField(new BStringField(type.c_str()), 1);
            row->SetField(new BStringField(address.c_str()), 2);
            row->SetField(new BStringField(sent.c_str()), 3);
            row->SetField(new BStringField(recv.c_str()), 4);
            row->SetField(new BStringField(tx.c_str()), 5);
            row->SetField(new BStringField(rx.c_str()), 6);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Baseline (New Allocations): " << elapsed.count() << " seconds" << std::endl;

    // Cleanup
    for (auto& pair : rowMap) delete pair.second;
}

void BenchmarkOptimized() {
    std::map<std::string, BRow*> rowMap;
    // Setup initial rows
    for (int i = 0; i < kNumInterfaces; ++i) {
        std::string name = "eth" + std::to_string(i);
        rowMap[name] = new BRow();
        // Pre-populate fields for optimized path to work
        BRow* row = rowMap[name];
        row->SetField(new BStringField(""), 1);
        row->SetField(new BStringField(""), 2);
        row->SetField(new BStringField(""), 3);
        row->SetField(new BStringField(""), 4);
        row->SetField(new BStringField(""), 5);
        row->SetField(new BStringField(""), 6);
    }

    auto start = std::chrono::high_resolution_clock::now();

    for (int iter = 0; iter < kNumIterations; ++iter) {
        for (int i = 0; i < kNumInterfaces; ++i) {
            std::string name = "eth" + std::to_string(i);

             // Simulate values
            std::string type = "Ethernet";
            std::string address = "192.168.1." + std::to_string(i);
            std::string sent = std::to_string(iter * 100) + " B";
            std::string recv = std::to_string(iter * 200) + " B";
            std::string tx = "10 KB/s";
            std::string rx = "20 KB/s";

            BRow* row = rowMap[name];

            // Optimized: Reuse field
            static_cast<BStringField*>(row->GetField(1))->SetString(type.c_str());
            static_cast<BStringField*>(row->GetField(2))->SetString(address.c_str());
            static_cast<BStringField*>(row->GetField(3))->SetString(sent.c_str());
            static_cast<BStringField*>(row->GetField(4))->SetString(recv.c_str());
            static_cast<BStringField*>(row->GetField(5))->SetString(tx.c_str());
            static_cast<BStringField*>(row->GetField(6))->SetString(rx.c_str());
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Optimized (Reuse Fields):   " << elapsed.count() << " seconds" << std::endl;

    // Cleanup
    for (auto& pair : rowMap) delete pair.second;
}

int main() {
    printf("Running benchmark with %d interfaces and %d iterations...\n", kNumInterfaces, kNumIterations);
    BenchmarkBaseline();
    BenchmarkOptimized();
    return 0;
}
