#include <iostream>
#include <chrono>
#include <vector>

typedef long long bigtime_t;
typedef long long int64;
typedef int int32;
typedef unsigned int uint32;

struct data_item {
	bigtime_t	time;
	int64		value;
	long long	seq;
};

// Simplified CircularBuffer for benchmark
template<typename Type>
class CircularBuffer {
public:
	CircularBuffer(uint32 size) : fSize(size), fIn(0), fFirst(0) {
		fBuffer = new Type[size];
	}
	~CircularBuffer() { delete[] fBuffer; }

	void AddItem(const Type& item) {
		uint32 index;
		if (fIn < fSize) {
			index = (fFirst + fIn) % fSize;
			fIn++;
		} else {
			index = fFirst;
			fFirst = (fFirst + 1) % fSize;
		}
		fBuffer[index] = item;
	}

	uint32 CountItems() const { return fIn; }

	Type* ItemAt(uint32 index) const {
		if (index >= fIn || fBuffer == nullptr || fSize == 0) return nullptr;
		return &fBuffer[(fFirst + index) % fSize];
	}

private:
	uint32 fSize;
	uint32 fIn;
	uint32 fFirst;
	Type* fBuffer;
};

class DataHistory {
public:
    DataHistory() : fBuffer(600) {}

    void AddValue(bigtime_t time, int64 value) {
        data_item item = {time, value, 0};
        fBuffer.AddItem(item);
    }

    int64 ValueAt(bigtime_t time, int32* hintIndex) {
        int32 left = 0;
        if (hintIndex != nullptr && *hintIndex >= 0)
            left = *hintIndex;

        int32 right = (int32)fBuffer.CountItems() - 1;
        if (left > right)
            return 0;

        // Fast path
        data_item* item = fBuffer.ItemAt(left);
        if (item != nullptr && item->time <= time) {
            data_item* nextItem = fBuffer.ItemAt(left + 1);
            if (nextItem == nullptr || nextItem->time > time) {
                if (nextItem == nullptr)
                    return item->value;

                int64 value = item->value;
                if (nextItem->time > item->time) {
                    value += static_cast<int64>(static_cast<double>(nextItem->value - value)
                        / (nextItem->time - item->time) * (time - item->time));
                }
                return value;
            } else {
                int32 nextIndex = left + 1;
                item = nextItem;
                nextItem = fBuffer.ItemAt(nextIndex + 1);

                if (nextItem == nullptr || nextItem->time > time) {
                    if (hintIndex != nullptr)
                        *hintIndex = nextIndex;

                    if (nextItem == nullptr)
                        return item->value;

                    int64 value = item->value;
                    if (nextItem->time > item->time) {
                        value += static_cast<int64>(static_cast<double>(nextItem->value - value)
                            / (nextItem->time - item->time) * (time - item->time));
                    }
                    return value;
                }
            }
        }

        while (left <= right) {
            int32 index = (left + right) / 2;
            item = fBuffer.ItemAt(index);

            if (item->time > time) {
                right = index - 1;
            } else {
                data_item* nextItem = fBuffer.ItemAt(index + 1);
                if (nextItem == nullptr) {
                    if (hintIndex != nullptr) *hintIndex = index;
                    return item->value;
                }
                if (nextItem->time > time) {
                    if (hintIndex != nullptr) *hintIndex = index;
                    int64 value = item->value;
                    if (nextItem->time > item->time) {
                        value += static_cast<int64>(static_cast<double>(nextItem->value - value)
                            / (nextItem->time - item->time) * (time - item->time));
                    }
                    return value;
                }
                left = index + 1;
            }
        }
        return 0;
    }

private:
    CircularBuffer<data_item> fBuffer;
};

int main() {
    DataHistory history;

    // Fill history with 600 items
    bigtime_t now = 1000000000000LL;
    bigtime_t interval = 1000000; // 1 second

    for (int i = 0; i < 600; i++) {
        history.AddValue(now + i * interval, i * 10);
    }

    int num_pixels = 1920;
    bigtime_t timeStep = 100000; // 0.1 seconds

    // Simulate ActivityGraphView draw loop queries
    auto start_orig = std::chrono::high_resolution_clock::now();
    int64 sum_orig = 0;
    for (int iter = 0; iter < 10000; iter++) {
        int32 searchIndex = 0;
        for (int i = 0; i < num_pixels; i++) {
            bigtime_t t = now + i * timeStep;
            sum_orig += history.ValueAt(t, &searchIndex);
        }
    }
    auto end_orig = std::chrono::high_resolution_clock::now();

    std::cout << "Duration: "
              << std::chrono::duration_cast<std::chrono::microseconds>(end_orig - start_orig).count()
              << " us" << std::endl;
    std::cout << "Sum (to prevent opt): " << sum_orig << std::endl;

    return 0;
}
