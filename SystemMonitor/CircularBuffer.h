/*
 * Copyright 2008-2013, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H


#include <stdlib.h>
#include <new>

#include <OS.h>


template<typename Type>
class CircularBuffer {
public:
	CircularBuffer(size_t size)
		:
		fSize(0),
		fBuffer(NULL)
	{
		SetSize(size);
	}

	~CircularBuffer()
	{
		delete[] fBuffer;
	}

	status_t InitCheck() const
	{
		return fBuffer != NULL ? B_OK : B_NO_MEMORY;
	}

	status_t SetSize(size_t size)
	{
		if (fSize == size)
			return B_OK;

		Type* newBuffer = new(std::nothrow) Type[size];
		if (newBuffer == NULL)
			return B_NO_MEMORY;

		if (fBuffer != NULL) {
			// Copy existing items
			uint32 count = fIn;
			if (count > size)
				count = size;

			for (uint32 i = 0; i < count; i++) {
				// We want the newest items if shrinking, or all if growing
				// But CircularBuffer logic usually appends.
				// If we shrink, we likely want the *latest* N items.
				// ItemAt(0) is the oldest. ItemAt(count-1) is the newest.
				// If we have 100 items and resize to 50, we want items 50..99.

				int32 sourceIndex;
				if (fIn > size)
					sourceIndex = i + (fIn - size); // Skip oldest
				else
					sourceIndex = i;

				Type* item = ItemAt(sourceIndex);
				if (item)
					newBuffer[i] = *item;
			}

			fIn = count;
			fFirst = 0;
		} else {
			fIn = 0;
			fFirst = 0;
		}

		delete[] fBuffer;
		fSize = size;
		fBuffer = newBuffer;

		return B_OK;
	}

	void MakeEmpty()
	{
		fIn = 0;
		fFirst = 0;
	}

	bool IsEmpty() const
	{
		return fIn == 0;
	}

	int32 CountItems() const
	{
		return fIn;
	}

	Type* ItemAt(int32 index) const
	{
		if (index >= (int32)fIn || index < 0 || fBuffer == NULL)
			return NULL;

		return &fBuffer[(fFirst + index) % fSize];
	}

	void AddItem(const Type& item)
	{
		uint32 index;
		if (fIn < fSize) {
			index = fFirst + fIn++;
		} else {
			index = fFirst;
			fFirst = (fFirst + 1) % fSize;
		}

		if (fBuffer != NULL)
			fBuffer[index % fSize] = item;
	}

	size_t Size() const
	{
		return fSize;
	}

private:
	CircularBuffer(const CircularBuffer& other);
	CircularBuffer& operator=(const CircularBuffer& other);

	uint32		fFirst;
	uint32		fIn;
	uint32		fSize;
	Type*		fBuffer;
};


#endif	// CIRCULAR_BUFFER_H
