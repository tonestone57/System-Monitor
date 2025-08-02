#include "DataHistory.h"

DataHistory::DataHistory(bigtime_t memorize, bigtime_t interval)
	:
	fBuffer(memorize > 0 && interval > 0 ? memorize / interval : 100),
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
	bool wasEmpty = fBuffer.IsEmpty();
	data_item* oldest = NULL;
	if (fBuffer.CountItems() == fBuffer.Size())
		oldest = fBuffer.ItemAt(0);

	data_item item = {time, value};
	fBuffer.AddItem(item);

	if (wasEmpty) {
		fMinimumValue = value;
		fMaximumValue = value;
		return;
	}

	if (value < fMinimumValue)
		fMinimumValue = value;
	if (value > fMaximumValue)
		fMaximumValue = value;

	if (oldest != NULL && (oldest->value == fMinimumValue || oldest->value == fMaximumValue))
		_RecalculateMinMax();
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


#include <limits.h>


void
DataHistory::SetRefreshInterval(bigtime_t interval)
{
	// TODO: adjust buffer size
}


void
DataHistory::_RecalculateMinMax()
{
	fMinimumValue = LLONG_MAX;
	fMaximumValue = LLONG_MIN;
	for (int32 i = 0; i < fBuffer.CountItems(); i++) {
		data_item* item = fBuffer.ItemAt(i);
		if (item->value < fMinimumValue)
			fMinimumValue = item->value;
		if (item->value > fMaximumValue)
			fMaximumValue = item->value;
	}
}
