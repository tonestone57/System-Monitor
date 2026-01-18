#ifndef DATAHISTORY_H
#define DATAHISTORY_H

#include <OS.h>
#include "CircularBuffer.h"

struct data_item {
	bigtime_t	time;
	int64		value;
};

class DataHistory {
public:
						DataHistory(bigtime_t memorize, bigtime_t interval);
						~DataHistory();

			void		AddValue(bigtime_t time, int64 value);

			int64		ValueAt(bigtime_t time);
			int64		MaximumValue() const;
			int64		MinimumValue() const;
			bigtime_t	Start() const;
			bigtime_t	End() const;

			void		SetRefreshInterval(bigtime_t interval);

private:
			void		_RecalculateMinMax();

private:
	CircularBuffer<data_item> fBuffer;
	int64				fMinimumValue;
	int64				fMaximumValue;
	bigtime_t			fRefreshInterval;
	bigtime_t			fMemorizeTime;
	int32				fLastIndex;
};

#endif // DATAHISTORY_H
