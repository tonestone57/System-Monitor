#include "DataHistory.h"

DataHistory::DataHistory(bigtime_t memorize, bigtime_t interval)
	:
	fBuffer(10000),
	fMinimumValue(0),
	fMaximumValue(0),
	fRefreshInterval(interval),
	fLastIndex(-1)
{
}


DataHistory::~DataHistory()
{
}


void
DataHistory::AddValue(bigtime_t time, int64 value)
{
	if (fBuffer.IsEmpty() || fMaximumValue < value)
		fMaximumValue = value;
	if (fBuffer.IsEmpty() || fMinimumValue > value)
		fMinimumValue = value;

	data_item item = {time, value};
	fBuffer.AddItem(item);
}


int64
DataHistory::ValueAt(bigtime_t time)
{
	int32 left = 0;
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
			if (nextItem == NULL)
				return item->value;
			if (nextItem->time > time) {
				// found item
				int64 value = item->value;
				value += int64(double(nextItem->value - value)
					/ (nextItem->time - item->time) * (time - item->time));
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
	return fMaximumValue;
}


int64
DataHistory::MinimumValue() const
{
	return fMinimumValue;
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
	// TODO: adjust buffer size
}
