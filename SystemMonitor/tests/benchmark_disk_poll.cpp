#include <iostream>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <string>

// Mock classes
class BString {
public:
	BString() {}
	BString(const char* str) : data(str) {}
	BString(const BString& other) : data(other.data) {
		// simulate some overhead
		volatile int x = 0;
		for(int i = 0; i < 50; ++i) x += i;
	}
	BString& operator=(const BString& other) {
		data = other.data;
		return *this;
	}
private:
	std::string data;
};

typedef uint64_t uint64;

struct DiskInfo {
	BString deviceName;
	BString mountPoint;
	BString fileSystemType;
	uint64 totalSize;
	uint64 freeSize;
	dev_t deviceID;
};

int main() {
	const int NUM_VOLUMES = 10; // Realistic number of volumes
	const int ITERATIONS = 100000;

	std::unordered_map<dev_t, DiskInfo> fVolumeCache;

	// Populate mock cache
	for (int i = 0; i < NUM_VOLUMES; i++) {
		DiskInfo info;
		info.deviceID = i;
		info.deviceName = "Device Name with some characters";
		info.mountPoint = "/boot/system/packages/something";
		info.fileSystemType = "bfs";
		info.totalSize = 1000000;
		info.freeSize = 500000;
		fVolumeCache[i] = info;
	}

	// Benchmark 1: Current approach (copying full DiskInfo objects)
	auto start1 = std::chrono::high_resolution_clock::now();
	uint64 totalSizeDummy1 = 0;
	for (int i = 0; i < ITERATIONS; i++) {
		std::vector<DiskInfo> volumesToPoll;
		for (auto const& pair : fVolumeCache) {
			volumesToPoll.push_back(pair.second);
		}

		for (auto& info : volumesToPoll) {
			// Simulate fs_stat_dev modifying it
			info.totalSize += 1;
			totalSizeDummy1 += info.totalSize;
		}

		// simulate updating back
		for (const auto& info : volumesToPoll) {
			fVolumeCache[info.deviceID] = info;
		}
	}
	auto end1 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> diff1 = end1 - start1;

	// Benchmark 2: Optimized approach (copying only dev_t, updating cache directly)
	auto start2 = std::chrono::high_resolution_clock::now();
	uint64 totalSizeDummy2 = 0;
	for (int i = 0; i < ITERATIONS; i++) {
		std::vector<dev_t> volumesToPoll;
		for (auto const& pair : fVolumeCache) {
			volumesToPoll.push_back(pair.first);
		}

		for (auto dev : volumesToPoll) {
			// Simulate fs_stat_dev
			fVolumeCache[dev].totalSize += 1;
			totalSizeDummy2 += fVolumeCache[dev].totalSize;
		}
	}
	auto end2 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> diff2 = end2 - start2;

	std::cout << "Baseline (Copying DiskInfo): " << diff1.count() << " ms" << std::endl;
	std::cout << "Optimized (Copying dev_t):   " << diff2.count() << " ms" << std::endl;
	std::cout << "Dummy values (prevent optimization): " << totalSizeDummy1 << ", " << totalSizeDummy2 << std::endl;

	return 0;
}
