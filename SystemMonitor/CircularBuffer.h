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

	CircularBuffer(const CircularBuffer& other)
		:
		fSize(0),
		fBuffer(NULL)
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
		if (other.fSize > 0) {
			newBuffer = new(std::nothrow) Type[other.fSize];
			if (newBuffer == NULL) {
				// Allocation failed, and we needed a buffer.
				// Retain old state.
				return *this;
			}

			// Copy data from other to newBuffer
			for (size_t i = 0; i < other.fSize; i++)
				newBuffer[i] = other.fBuffer[i];
		}

		delete[] fBuffer;
		fBuffer = newBuffer;
		fSize = other.fSize;
		fFirst = other.fFirst;
		fIn = other.fIn;

		return *this;
	}

	status_t InitCheck() const
	{
		return fBuffer != NULL ? B_OK : B_NO_MEMORY;
	}

	status_t SetSize(size_t size)
	{
		if (fSize == size)
			return B_OK;

		if (size == 0) {
			delete[] fBuffer;
			fBuffer = NULL;
			fSize = 0;
			fFirst = 0;
			fIn = 0;
			return B_OK;
		}

		Type* newBuffer = new(std::nothrow) Type[size];
		if (newBuffer == NULL)
			return B_NO_MEMORY;

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
		if (index >= (int32)fIn || index < 0 || fBuffer == NULL || fSize == 0)
			return NULL;

		return &fBuffer[(fFirst + index) % fSize];
	}

	void AddItem(const Type& item)
	{
		if (fSize == 0)
			return;

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
	uint32		fFirst;
	uint32		fIn;
	uint32		fSize;
	Type*		fBuffer;
};


#endif	// CIRCULAR_BUFFER_H
