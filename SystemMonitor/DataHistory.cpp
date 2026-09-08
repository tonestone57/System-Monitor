#include "DataHistory.h"
#include <limits.h>

static uint32
CalculateInitialBufferSize(bigtime_t memorize, bigtime_t interval)
{
	if (memorize <= 0 || interval <= 0)
		return 100;
	uint32 size = static_cast<uint32>(memorize / interval);
	return size == 0 ? 1 : size;
}

DataHistory::DataHistory(bigtime_t memorize, bigtime_t interval)
	:
	fBuffer(CalculateInitialBufferSize(memorize, interval)),
	fRefreshInterval(interval),
	fNextSeq(0)
{
}


DataHistory::~DataHistory()
{
}


void
DataHistory::AddValue(bigtime_t time, int64 value)
{
	bool full = fBuffer.Size() > 0 && static_cast<size_t>(fBuffer.CountItems()) == fBuffer.Size();
	uint64 oldestSeq = 0;
	if (full) {
		data_item* oldest = fBuffer.ItemAt(0);
		if (oldest != NULL)
			oldestSeq = oldest->seq;
	}

	data_item item = {time, value, fNextSeq++};
	fBuffer.AddItem(item);

	// Maintain Min Deque (increasing)
	while (!fMinDeque.empty() && fMinDeque.back().value >= value) {
		fMinDeque.pop_back();
	}
	fMinDeque.push_back(item);

	// Maintain Max Deque (decreasing)
	while (!fMaxDeque.empty() && fMaxDeque.back().value <= value) {
		fMaxDeque.pop_back();
	}
	fMaxDeque.push_back(item);

	if (full) {
		if (!fMinDeque.empty() && fMinDeque.front().seq == oldestSeq)
			fMinDeque.pop_front();

		if (!fMaxDeque.empty() && fMaxDeque.front().seq == oldestSeq)
			fMaxDeque.pop_front();
	}
}


int64
DataHistory::ValueAt(bigtime_t time, int32* hintIndex)
{
	int32 right = (int32)fBuffer.CountItems() - 1;
	if (right < 0)
		return 0;

	int32 left = 0;
	if (hintIndex != NULL && *hintIndex >= 0 && *hintIndex <= right)
		left = *hintIndex;

	// Fast path: sequentially progressing time is often in the same or next interval
	data_item* item = fBuffer.ItemAt(left);
	if (item != NULL && item->time <= time) {
		data_item* nextItem = fBuffer.ItemAt(left + 1);
		if (nextItem == NULL || nextItem->time > time) {
			// Found in the current interval [left, left+1)
			if (hintIndex != NULL)
				*hintIndex = left;

			if (nextItem == NULL)
				return item->value;

			int64 value = item->value;
			if (nextItem->time > item->time) {
				value += static_cast<int64>(static_cast<double>(nextItem->value - value)
					/ (nextItem->time - item->time) * (time - item->time));
			}
			return value;
		} else {
			// Might be in the very next interval [left+1, left+2)
			int32 nextIndex = left + 1;
			item = nextItem;
			nextItem = fBuffer.ItemAt(nextIndex + 1);

			if (nextItem == NULL || nextItem->time > time) {
				if (hintIndex != NULL)
					*hintIndex = nextIndex;

				if (nextItem == NULL)
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

	left = 0;
	while (left <= right) {
		int32 index = (left + right) / 2;
		item = fBuffer.ItemAt(index);
		if (item == NULL)
			return 0;

		if (item->time > time) {
			// search in left part
			right = index - 1;
		} else {
			data_item* nextItem = fBuffer.ItemAt(index + 1);
			if (nextItem == NULL) {
				if (hintIndex != NULL)
					*hintIndex = index;
				return item->value;
			}
			if (nextItem->time > time) {
				// found item
				if (hintIndex != NULL)
					*hintIndex = index;

				int64 value = item->value;
				// Prevent division by zero if multiple samples have the same timestamp
				if (nextItem->time > item->time) {
					value += static_cast<int64>(static_cast<double>(nextItem->value - value)
						/ (nextItem->time - item->time) * (time - item->time));
				}
				return value;
			}

			// search in right part
			left = index + 1;
		}
	}

	return 0;
}


void
DataHistory::GetValues(int64* outValues, int32 count, bigtime_t startTime, bigtime_t timeStep)
{
	if (outValues == NULL || count <= 0) return;

	int32 right = (int32)fBuffer.CountItems() - 1;
	if (right < 0) {
		for (int i = 0; i < count; ++i) outValues[i] = 0;
		return;
	}

	// Fast path: if startTime is way after the last item
	data_item* lastItem = fBuffer.ItemAt(right);
	if (lastItem != NULL && lastItem->time <= startTime) {
		for (int i = 0; i < count; ++i) {
			outValues[i] = lastItem->value;
		}
		return;
	}

	int32 index = 0;
	int32 left = 0;
	int32 r = right;
	// Binary search to find the item right before or exactly at startTime
	while (left <= r) {
		int32 mid = (left + r) / 2;
		data_item* item = fBuffer.ItemAt(mid);
		if (item != NULL && item->time > startTime) {
			r = mid - 1;
		} else {
			if (item != NULL)
				index = mid;
			left = mid + 1;
		}
	}

	data_item* item = fBuffer.ItemAt(index);
	data_item* nextItem = fBuffer.ItemAt(index + 1);

	for (int32 i = 0; i < count; i++) {
		bigtime_t time = startTime + static_cast<bigtime_t>(i) * timeStep;

		while (nextItem != NULL && nextItem->time <= time) {
			index++;
			item = nextItem;
			nextItem = fBuffer.ItemAt(index + 1);
		}

		if (item == NULL) {
			outValues[i] = 0;
		} else if (item->time > time) {
			outValues[i] = 0;
		} else if (nextItem == NULL) {
			outValues[i] = item->value;
		} else {
			int64 value = item->value;
			int64 timeDiff = nextItem->time - item->time;
			if (timeDiff > 0) {
				value += static_cast<int64>(static_cast<double>(nextItem->value - value)
					/ timeDiff * (time - item->time));
			}
			outValues[i] = value;
		}
	}
}


int64
DataHistory::MaximumValue() const
{
	if (fMaxDeque.empty())
		return 0;
	return fMaxDeque.front().value;
}


int64
DataHistory::MinimumValue() const
{
	if (fMinDeque.empty())
		return 0;
	return fMinDeque.front().value;
}


bigtime_t
DataHistory::Start() const
{
	if (fBuffer.CountItems() == 0)
		return 0;

	return fBuffer.ItemAt(0)->time;
}


bigtime_t
DataHistory::End() const
{
	if (fBuffer.CountItems() == 0)
		return 0;

	return fBuffer.ItemAt(fBuffer.CountItems() - 1)->time;
}


void
DataHistory::SetRefreshInterval(bigtime_t interval)
{
	if (interval <= 0 || interval == fRefreshInterval)
		return;

	// Calculate current duration with old interval
	bigtime_t duration = fBuffer.Size() * fRefreshInterval;

	// Calculate new size to keep the same duration
	size_t newSize = duration / interval;
	if (newSize < 10) newSize = 10;

	if (fBuffer.SetSize(newSize) == B_OK) {
		fRefreshInterval = interval;
		// Re-stamp sequence numbers after resize so deque eviction remains correct
		uint32 count = fBuffer.CountItems();
		fNextSeq = count;
		for (uint32 i = 0; i < count; i++) {
			data_item* item = fBuffer.ItemAt(i);
			if (item != NULL)
				item->seq = i;
		}
		_ResetDeques();
	}
}


void
DataHistory::_ResetDeques()
{
	fMinDeque.clear();
	fMaxDeque.clear();

	uint32 count = fBuffer.CountItems();
	for (uint32 i = 0; i < count; i++) {
		data_item* item = fBuffer.ItemAt(i);
		if (item == NULL) continue;

		int64 value = item->value;

		// Maintain Min Deque
		while (!fMinDeque.empty() && fMinDeque.back().value >= value) {
			fMinDeque.pop_back();
		}
		fMinDeque.push_back(*item);

		// Maintain Max Deque
		while (!fMaxDeque.empty() && fMaxDeque.back().value <= value) {
			fMaxDeque.pop_back();
		}
		fMaxDeque.push_back(*item);
	}
}
