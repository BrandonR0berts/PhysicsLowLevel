#pragma once

#include <malloc.h>
#include <iostream>
#include <assert.h>

#include <mutex>

namespace Memory
{
	// ----------------------------------------------------------

	struct MemoryMetaData
	{
		MemoryMetaData()
			: mUsed(false)
		{ }

		// Holds size and used/unused
		bool mUsed;
	};

	struct MemoryBlock
	{
		MemoryMetaData mMetaData;
		char           mData[1];
	};

	// x86 architecture = word size of 4
	static inline unsigned int AlignToArchitecture(size_t n)
	{
		// (n + sizeof(word) - 1) & ~(sizeof(word) - 1)
		// Move count into next word - crop result back down to the word size
		return (n + 3) & ~(3);
	}

	struct LargeDataMemoryBlock
	{
		LargeDataMemoryBlock()
			: mNext(nullptr)
			, mPrior(nullptr)
			, mDataSizeAndUsed(0)
			, mData()
		{ }

		LargeDataMemoryBlock(unsigned int totalSize, bool inUse)
			: mNext(nullptr)
			, mPrior(nullptr)
			, mDataSizeAndUsed(totalSize)
			, mData()
		{
			// Set the flag if the data is in use
			if (inUse)
				mDataSizeAndUsed |= 1;
		}

		LargeDataMemoryBlock* mNext;
		LargeDataMemoryBlock* mPrior;

		// LSB is 1 or 0 telling if the block is used or not
		// This is possible because we are aligning to the architecture blocks
		// as the LSB is not actually ever used to store size
		size_t                mDataSizeAndUsed;

		char                  mData[1];
	};

	struct FreeMemoryBlockInfo
	{
		LargeDataMemoryBlock* mAddress;
	};

	struct ArraySizingData
	{
		ArraySizingData(void* address, size_t size)
			: mMemoryAddress(address)
			, mSize(size)
		{ }

		void*  mMemoryAddress; // This is the pointer provided to the user to use, not the start of the meta data struct
		size_t mSize;          // This is the entire size, from the start of the meta data all the way to the end of the user provided data (plus any extra for alignment)
	};

	// ----------------------------------------------------------

	constexpr unsigned int kMinSizeForLargeAllocationBlock = 40;

	// Set to 10MB currently
	constexpr unsigned int kBytesAllocatedForLargeAllocations = 1024 * 1024 * 1024;
	constexpr unsigned int kBytesAllocatedForFreeArray        = 1024 * 1024;
	
	// ----------------------------------------------------------

	class MemoryPool
	{
	public:
		MemoryPool();
		~MemoryPool();

		void* AssignMemory(size_t size, bool forArray);
		void  FreeMemory(size_t size, void* memoryPointer, bool forArray);

		void  Init();
		void  InitMutex();

		static MemoryPool* Get()
		{
			if (!mThis)
			{
				mThis  = (MemoryPool*)malloc(sizeof(MemoryPool));

				if(mThis)
					*mThis = MemoryPool();
			}

			return mThis; 
		}

		void DebugOutputUsage(bool outputPreSized = true, bool outputLargeAllocations = true);

		std::mutex* GetMutex() { return mBlockingMutex; }

	private:

		static MemoryPool* mThis;

		// ---------------------------------------------------------------------- //

		LargeDataMemoryBlock* mLargeDataAllocationsList;       // For looping through the list
		LargeDataMemoryBlock* mLastElementInLargeAllocations;  // For fast insertion of new elements to the list
		bool                  mLargeAllocationsPopulated;      // If the first element in the list is valid / anything has been added to list

		FreeMemoryBlockInfo*  mFreeLargeElementsList;          // An array holding information about free elements in the larger list above
		unsigned int          mFreeElementsAllocated;

		ArraySizingData*      mSavedArraySizes;
		unsigned int          mSavedArraySizesCount;

		std::mutex*           mBlockingMutex;

		unsigned int          mMemoryUsed;

		// ---------------------------------------------------------------------- //
	};

	// ----------------------------------------------------------
}