/*
 * Copyright 2008-2013, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H


#include <stdlib.h>
#include <new>

#if defined(__HAIKU__) || defined(BEOS)
#include <OS.h>
#endif


template<typename Type>
class CircularBuffer {
public:
	CircularBuffer(uint32 size)
		:
		fFirst(0),
		fIn(0),
		fSize(0),
		fBuffer(NULL),
		fInitStatus(B_OK)
	{
		fInitStatus = SetSize(size);
	}

	CircularBuffer(const CircularBuffer& other)
		:
		fFirst(0),
		fIn(0),
		fSize(0),
		fBuffer(NULL),
		fInitStatus(B_OK)
	{
		*this = other;
	}

	~CircularBuffer()
	{
		delete[] fBuffer;
	}

	CircularBuffer& operator=(const CircularBuffer& other)
	{
		if (this == &other)
			return *this;

		Type* newBuffer = NULL;
		if (other.fSize > 0 && other.fBuffer != NULL) {
			newBuffer = new(std::nothrow) Type[other.fSize];
			if (newBuffer == NULL) {
				// Allocation failed, and we needed a buffer.
				// Retain old state and update init status.
				fInitStatus = B_NO_MEMORY;
				return *this;
			}

			for (uint32 i = 0; i < other.fSize; i++) {
				newBuffer[i] = other.fBuffer[i];
			}
			fFirst = other.fFirst;
			fIn = other.fIn;
		} else {
			fFirst = 0;
			fIn = 0;
		}

		delete[] fBuffer;
		fBuffer = newBuffer;
		fSize = other.fSize;
		fInitStatus = B_OK;

		return *this;
	}

	status_t InitCheck() const
	{
		return fInitStatus;
	}

	status_t SetSize(uint32 size)
	{
		if (fSize == size && fInitStatus == B_OK)
			return B_OK;

		if (size == 0) {
			delete[] fBuffer;
			fBuffer = NULL;
			fSize = 0;
			fFirst = 0;
			fIn = 0;
			fInitStatus = B_OK;
			return B_OK;
		}

		Type* newBuffer = new(std::nothrow) Type[size];
		if (newBuffer == NULL) {
			fInitStatus = B_NO_MEMORY;
			return B_NO_MEMORY;
		}

		if (fBuffer != NULL && fSize > 0) {
			// Preserve existing data
			uint32 itemsToCopy = (fIn < size) ? fIn : size;
			uint32 sourceIndex = fFirst;
			// If we are shrinking and have more items than new size, we drop oldest
			if (fIn > size) {
				sourceIndex = (fFirst + (fIn - size)) % fSize;
			}

			for (uint32 i = 0; i < itemsToCopy; i++) {
				newBuffer[i] = fBuffer[(sourceIndex + i) % fSize];
			}

			fFirst = 0;
			fIn = itemsToCopy;
		} else {
			fFirst = 0;
			fIn = 0;
		}

		delete[] fBuffer;
		fBuffer = newBuffer;
		fSize = size;
		fInitStatus = B_OK;

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

	uint32 CountItems() const
	{
		return fIn;
	}

	Type* ItemAt(uint32 index) const
	{
		if (index >= fIn || fBuffer == NULL || fSize == 0)
			return NULL;

		return &fBuffer[(fFirst + index) % fSize];
	}

	void AddItem(const Type& item)
	{
		if (fBuffer == NULL || fSize == 0)
			return;

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

	uint32 Size() const
	{
		return fSize;
	}

private:
	uint32		fFirst;
	uint32		fIn;
	uint32		fSize;
	Type*		fBuffer;
	status_t	fInitStatus;
};


#endif	// CIRCULAR_BUFFER_H
