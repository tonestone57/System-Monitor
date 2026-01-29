#ifndef DATAHISTORY_H
#define DATAHISTORY_H

#include <OS.h>
#include <deque>
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
			void		_ResetDeques();

private:
	CircularBuffer<data_item> fBuffer;
	std::deque<data_item> fMinDeque;
	std::deque<data_item> fMaxDeque;
	bigtime_t			fRefreshInterval;
	int32				fLastIndex;
};

#endif // DATAHISTORY_H
