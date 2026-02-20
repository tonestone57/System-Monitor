#include "DataHistory.h"
#include <limits.h>

DataHistory::DataHistory(bigtime_t memorize, bigtime_t interval)
	:
	fBuffer(memorize > 0 && interval > 0 ? memorize / interval : 100),
	fRefreshInterval(interval)
{
}


DataHistory::~DataHistory()
{
}


void
DataHistory::AddValue(bigtime_t time, int64 value)
{
	bool full = static_cast<size_t>(fBuffer.CountItems()) == fBuffer.Size();
	bigtime_t oldestTime = 0;
	if (full) {
		data_item* oldest = fBuffer.ItemAt(0);
		if (oldest != NULL)
			oldestTime = oldest->time;
	}

	data_item item = {time, value};
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
		if (!fMinDeque.empty() && fMinDeque.front().time == oldestTime)
			fMinDeque.pop_front();

		if (!fMaxDeque.empty() && fMaxDeque.front().time == oldestTime)
			fMaxDeque.pop_front();
	}
}


int64
DataHistory::ValueAt(bigtime_t time, int32* hintIndex)
{
	int32 left = 0;
	if (hintIndex != NULL && *hintIndex >= 0)
		left = *hintIndex;

	int32 right = fBuffer.CountItems() - 1;
	data_item* item = NULL;

	while (left <= right) {
		int32 index = (left + right) / 2;
		item = fBuffer.ItemAt(index);

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
		_ResetDeques();
	}
}


void
DataHistory::_ResetDeques()
{
	fMinDeque.clear();
	fMaxDeque.clear();

	int32 count = fBuffer.CountItems();
	for (int32 i = 0; i < count; i++) {
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
