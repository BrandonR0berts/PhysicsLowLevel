#pragma once

#include "Commons.h"

// ------------------------------------------------

#if MemoryOverride

	void* operator new(size_t size);
	void operator delete(void* pointer, size_t size);

#endif

// ------------------------------------------------

struct Header final
{
	Header()
		: mCheckValue(ChecksumValue)
		, mBytesInMemory(0)
		, mNextHeader(nullptr)
		, mPriorHeader(nullptr)
	{ }

	Header(unsigned int bytesInMemory, Header* nextHeader, Header* priorHeader)
		: mCheckValue(ChecksumValue)
		, mBytesInMemory(bytesInMemory)
		, mNextHeader(nextHeader)
		, mPriorHeader(priorHeader)
	{ }

	unsigned int mCheckValue;
	unsigned int mBytesInMemory;
	Header*      mNextHeader;
	Header*      mPriorHeader;
};

// ------------------------------------------------

struct Footer final
{
	Footer()
		: mCheckValue(ChecksumValue)
	{ }

	unsigned int mCheckValue;
};

// ------------------------------------------------

class BaseTracker
{
public:
	BaseTracker();
	BaseTracker(Header* firstMemoryAddress);
	~BaseTracker();

	bool WalkTheHeap();

	void AddMemoryAllocated(Header* header);
	void RemoveMemoryAllocated(Header* header);

private:
	unsigned int mBytesAllocated;
	Header*      mFirstMemoryAddress;
	Header*      mLastMemoryAddress;
};

// ------------------------------------------------