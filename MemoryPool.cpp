#include "MemoryPool.h"

#include <assert.h>
#include <memory.h>

namespace Memory
{
	MemoryPool* MemoryPool::mThis = nullptr;

	// -------------------------------------------------------------------------

	MemoryPool::MemoryPool()
		: mLargeDataAllocationsList(nullptr)
		, mLastElementInLargeAllocations(nullptr)
		, mLargeAllocationsPopulated(false)

		, mFreeLargeElementsList(nullptr)
		, mFreeElementsAllocated(0)

		, mSavedArraySizes(nullptr)
		, mSavedArraySizesCount(0)

		, mBlockingMutex(nullptr)

		, mMemoryUsed(0)
	{
		
	}

	// -------------------------------------------------------------------------

	MemoryPool::~MemoryPool()
	{
		delete mBlockingMutex;

		free(mLargeDataAllocationsList);
	}

	// -------------------------------------------------------------------------

	void MemoryPool::Init()
	{
		mLargeDataAllocationsList = (LargeDataMemoryBlock*)malloc(kBytesAllocatedForLargeAllocations);

		// Use the last 1MB for the free blocks list 
		mFreeLargeElementsList = (FreeMemoryBlockInfo*)(((char*)mLargeDataAllocationsList) + kBytesAllocatedForLargeAllocations - kBytesAllocatedForFreeArray);

		// Set all of the memory at the end to 0 (nullptr) so that we have something to check against as the array fills up
		// Shifted by 1 as we cannot store nullptr for the first element, as that would lose the pointer to the start of the array
		memset((char*)mFreeLargeElementsList + sizeof(FreeMemoryBlockInfo), 0, kBytesAllocatedForFreeArray - sizeof(FreeMemoryBlockInfo));

		// This is the 1MB before the free elements array
		mSavedArraySizes = (ArraySizingData*)(((char*)mFreeLargeElementsList) - kBytesAllocatedForFreeArray);
	}

	// -------------------------------------------------------------------------

	void MemoryPool::InitMutex()
	{
		mBlockingMutex = new std::mutex();
	}

	// -------------------------------------------------------------------------

	void* MemoryPool::AssignMemory(size_t size, bool forArray)
	{	
		// size = amount the caller wants
		// sizeof(LargeDataMemoryBlock) is the meta data + 1st byte of real memory
		// -1 to solve any overlap
		// Then make sure to have the data allocated be aligned to memory alignments
		size_t alignedSizeForLargeAllocation = AlignToArchitecture(size + sizeof(LargeDataMemoryBlock) - 1);

		// ----------------------------------------------- //

		// See if the head of the linked list is null
		if (mLargeAllocationsPopulated)
		{
			// ----------------------------------------------- //
				 
			// See if there are any free large blocks of memory that are the right size
			for (unsigned int i = 0; i < mFreeElementsAllocated; i++)
			{
				if (mFreeLargeElementsList[i].mAddress->mDataSizeAndUsed < alignedSizeForLargeAllocation)
					continue;

				LargeDataMemoryBlock* addressPointedTo = mFreeLargeElementsList[i].mAddress;

				if (addressPointedTo->mDataSizeAndUsed == alignedSizeForLargeAllocation || // Exact right size
					(int)((int)addressPointedTo->mDataSizeAndUsed - (int)alignedSizeForLargeAllocation) > (int)AlignToArchitecture(kMinSizeForLargeAllocationBlock + sizeof(LargeDataMemoryBlock) - 1))
				{
					// Store the size if this is for an array
					if (forArray)
					{
						mSavedArraySizes[mSavedArraySizesCount] = ArraySizingData(&addressPointedTo->mData[0], alignedSizeForLargeAllocation);

						mSavedArraySizesCount++;
					}

					// Now see if we need to add another free block which is the remainder of the memory within the block
					if (addressPointedTo->mDataSizeAndUsed != alignedSizeForLargeAllocation)
					{
						// Create a new large memory block at the split point
						LargeDataMemoryBlock* newBlock = (LargeDataMemoryBlock*)(((char*)addressPointedTo) + alignedSizeForLargeAllocation);
							                 *newBlock = LargeDataMemoryBlock(addressPointedTo->mDataSizeAndUsed - alignedSizeForLargeAllocation, false);

						// Make sure that the newly created slot has the right size
						addressPointedTo->mDataSizeAndUsed  = alignedSizeForLargeAllocation;
						addressPointedTo->mDataSizeAndUsed |= 1;

						// Instead of removing the free element, we can instead just adjust the address it points to to be beyond the space we just re-used
						mFreeLargeElementsList[i].mAddress = newBlock;

						// Now we need to add this memory block to the list
						LargeDataMemoryBlock* oldNext = addressPointedTo->mNext;

						addressPointedTo->mNext = newBlock;
						newBlock->mNext         = oldNext;

						// Set the prior pointer to being the one we just split in two
						newBlock->mPrior = addressPointedTo;

						// If this element was the last in the list
						if (oldNext == nullptr)
						{
							mLastElementInLargeAllocations = newBlock;
						}
						else
						{
							oldNext->mPrior = newBlock;
						}
					}
					else // Exactly equal to the amount of memory we need
					{
						// Set as used
						addressPointedTo->mDataSizeAndUsed |= 1;

						// See which element we are looking at and copy across the last element to be here as a replacement
						mFreeLargeElementsList[i].mAddress = mFreeLargeElementsList[mFreeElementsAllocated - 1].mAddress;
						mFreeElementsAllocated--;
					}

					// Return the memory address back so that the user can modify this data section
					return &addressPointedTo->mData[0];
				}
			}				

			// ----------------------------------------------- //

			// No free blocks - or none that match the size - so need to create a new one at the end

			// Grab the current last element and move along to the next free slot to construct a new container
			// Cast to a char for byte movement size
			unsigned int bytesInSizeAllocation = (mLastElementInLargeAllocations->mDataSizeAndUsed & ~1);
			LargeDataMemoryBlock* nextFreeSlot = (LargeDataMemoryBlock*)((char*)mLastElementInLargeAllocations + bytesInSizeAllocation);

			mMemoryUsed += alignedSizeForLargeAllocation;

			if (mMemoryUsed >= kBytesAllocatedForLargeAllocations - kBytesAllocatedForFreeArray)
			{
				assert("Out of memory" && false);
			}

			// Construct the data here
			*nextFreeSlot = LargeDataMemoryBlock(alignedSizeForLargeAllocation, true);

			// Make sure that the last element is this new one
			mLastElementInLargeAllocations->mNext = nextFreeSlot; // Set the old last one's next to be the new last one
			nextFreeSlot->mPrior                  = mLastElementInLargeAllocations;
			mLastElementInLargeAllocations        = nextFreeSlot; // Set the new last one to being the last one

			// Store the size if this is for an array
			if (forArray)
			{
				mSavedArraySizes[mSavedArraySizesCount] = ArraySizingData(&nextFreeSlot->mData[0], alignedSizeForLargeAllocation);

				mSavedArraySizesCount++;
			}

			// Return the new memory address for the user to use
			return &nextFreeSlot->mData[0];

			// ----------------------------------------------- //
		}
		else
		{
			// Construct the block at the start - no memory allocation needed
			*mLargeDataAllocationsList = LargeDataMemoryBlock(alignedSizeForLargeAllocation, true);

			mMemoryUsed += alignedSizeForLargeAllocation;

			if (mMemoryUsed >= kBytesAllocatedForLargeAllocations - kBytesAllocatedForFreeArray)
			{
				assert("Out of memory" && false);
			}

			// Set that we have one stored
			mLargeAllocationsPopulated = true;

			// Store this element as the last so we can build from here later on
			mLastElementInLargeAllocations = mLargeDataAllocationsList;

			// Save the size if this is for an array
			if (forArray)
			{
				mSavedArraySizes[mSavedArraySizesCount] = ArraySizingData(&mLargeDataAllocationsList->mData[0], alignedSizeForLargeAllocation);

				mSavedArraySizesCount++;
			}

			// Return the address of the start of the data we are allowing the user to modify
			return &mLargeDataAllocationsList->mData[0];
		}
 	}

	// -------------------------------------------------------------------------

	void MemoryPool::FreeMemory(size_t size, void* memoryPointer, bool forArray)
	{
		if (forArray)
		{
			bool foundSize = false;

			// If this is for an array then the size passed in will be 0, as we dont know the size right now
			// So look through the mapping of sizes to find the size
			for (unsigned int i = 0; i < mSavedArraySizesCount; i++)
			{
				if (mSavedArraySizes[i].mMemoryAddress == memoryPointer)
				{
					// Set the size we are dealing with for later in the function
					size = mSavedArraySizes[i].mSize;

					// Remove this element from the list 
					mSavedArraySizes[i] = mSavedArraySizes[mSavedArraySizesCount - 1];

					// Reduce count by 1
					mSavedArraySizesCount--;

					foundSize = true;

					break;
				}
			}

			if (!foundSize)
				assert(false);
		}

		// ----------------------------------------------- //

		// See if the head of the linked list is null
#ifdef _DEBUG
		if (!mLargeAllocationsPopulated)
		{
			assert(false);
			return;
		}
#endif

		// size = amount the caller has
		// sizeof(LargeDataMemoryBlock) is the meta data + 1st byte of real memory
		// -1 to solve any overlap
		// Then make sure to have the data allocated be aligned to memory alignments
		size_t alignedSizeForLargeAllocation = size;

		// Dont need to adjust for an array element as it has already been factored in
		if(!forArray)
			alignedSizeForLargeAllocation = AlignToArchitecture(size + sizeof(LargeDataMemoryBlock) - 1);

		// Jump to the right point in memory - the pointer passed in is the point of the start of the char[1], so we just need to move back the size of the header
		unsigned int          adjustmentCount     = sizeof(size_t) + (2 * sizeof(LargeDataMemoryBlock*));
		LargeDataMemoryBlock* memoryBlockPassedIn = (LargeDataMemoryBlock*)(((char*)memoryPointer) - adjustmentCount);

		if (memoryBlockPassedIn)
		{
			// Set the block to not being used
			memoryBlockPassedIn->mDataSizeAndUsed &= ~1;

			mMemoryUsed -= memoryBlockPassedIn->mDataSizeAndUsed;

			// this should always match, if not then we have stored the wrong value somewhere
#ifdef _DEBUG
			if (memoryBlockPassedIn->mDataSizeAndUsed != alignedSizeForLargeAllocation)
			{
				DebugOutputUsage();
				assert(false);
			}
#endif

			bool mergeForwards          = memoryBlockPassedIn->mNext  && !(memoryBlockPassedIn->mNext->mDataSizeAndUsed & 1);
			bool mergeBackwards         = memoryBlockPassedIn->mPrior && !(memoryBlockPassedIn->mPrior->mDataSizeAndUsed & 1);
			int  freeBlockIndexMergedTo = -1;

			// ------------------------------- Merge forwards  ------------------------------- //
			// See if we can merge this block with the one after it into one large block //
			if (mergeForwards)
			{
				// The next element in the list exists and is not being used
				memoryBlockPassedIn->mDataSizeAndUsed += memoryBlockPassedIn->mNext->mDataSizeAndUsed;

				// Make sure to remove the freeBlockList element pointing to the place we are now skipping over
				for (unsigned int i = 0; i < mFreeElementsAllocated; i++)
				{
					// TODO make this nicer to cache //
					if (mFreeLargeElementsList[i].mAddress == memoryBlockPassedIn->mNext)
					{
						// Remove this point by copying the last address in the list over this data
						mFreeLargeElementsList[i].mAddress = memoryBlockPassedIn;

						freeBlockIndexMergedTo = i;

						break;
					}
				}

				// If the one we have just merged into this is the last one in the list then we need to update the store of the last element
				if (memoryBlockPassedIn->mNext->mNext == nullptr)
					mLastElementInLargeAllocations = memoryBlockPassedIn;
				else
				{
					memoryBlockPassedIn->mNext->mNext->mPrior = memoryBlockPassedIn;
				}

				// Skip over the memory from the one that has been merged
				memoryBlockPassedIn->mNext = memoryBlockPassedIn->mNext->mNext;
			}

			// ------------------------------- Merge backwards  ------------------------------- //
			if (mergeBackwards)
			{
				// The prior point in memory exists and is not being used

				// Add this size to the prior element's size
				memoryBlockPassedIn->mPrior->mDataSizeAndUsed += memoryBlockPassedIn->mDataSizeAndUsed;

				// If we have just merged forwards, then both this and the prior point are stored in the free list,
				// So we need to make sure we fix that up
				if (mergeForwards && freeBlockIndexMergedTo != -1)
				{
					// Remove this point by copying the last address in the list over this data
					mFreeLargeElementsList[freeBlockIndexMergedTo].mAddress = mFreeLargeElementsList[mFreeElementsAllocated - 1].mAddress;

					mFreeElementsAllocated--;
				}

				// Check if this element is the last one, and if so then we need to adjust the store of the last element
				if (memoryBlockPassedIn->mNext == nullptr)
				{
					mLastElementInLargeAllocations = memoryBlockPassedIn->mPrior;
				}
				else
				{
					memoryBlockPassedIn->mNext->mPrior = memoryBlockPassedIn->mPrior;
				}

				// Skip over this element in the list
				memoryBlockPassedIn->mPrior->mNext = memoryBlockPassedIn->mNext;
			}

			// If not merging with anything then we need to add this block to the free list
			if (!mergeBackwards && !mergeForwards)
			{
				mFreeLargeElementsList[mFreeElementsAllocated]          = FreeMemoryBlockInfo();
				mFreeLargeElementsList[mFreeElementsAllocated].mAddress = memoryBlockPassedIn;
				mFreeElementsAllocated++;
			}

			return;
		}

		DebugOutputUsage();

		assert(false);	
	}

	// -------------------------------------------------------------------------

	void MemoryPool::DebugOutputUsage(bool outputPreSized, bool outputLargeAllocations)
	{	
		if (outputLargeAllocations)
		{
			std::cout << "Large data allocations" << std::endl;

			LargeDataMemoryBlock* currentBlock = mLargeDataAllocationsList;
			unsigned int          elementsInList = 0;

			unsigned int          bytesAllocated = 0;

			while (currentBlock != nullptr)
			{
				elementsInList++;

				bool used = currentBlock->mDataSizeAndUsed & 1 ? true : false;

				std::cout << "Address:\t" << currentBlock << "\tSize:\t" << (used ? currentBlock->mDataSizeAndUsed - 1 : currentBlock->mDataSizeAndUsed) << "\tIn use:\t" << (used ? "yes" : "no") << "\t";
				std::cout << "Next:\t" << currentBlock->mNext;
				std::cout << "\tPrior:\t" << currentBlock->mPrior << std::endl;

				if (currentBlock->mNext && currentBlock > currentBlock->mNext)
					std::cout << "Ordering issue!" << std::endl;

				bytesAllocated += (currentBlock->mDataSizeAndUsed & ~1);

				currentBlock = currentBlock->mNext;
			}
			std::cout << "Elements in list: " << elementsInList << std::endl << std::endl;

			std::cout << "Total bytes in memory pool: " << kBytesAllocatedForLargeAllocations << std::endl;
			std::cout << "Bytes allocated to program: " << bytesAllocated << std::endl;

			// Free slots list
			std::cout << "List of free sizes:" << std::endl;
			for (unsigned int i = 0; i < mFreeElementsAllocated; i++)
			{
				std::cout << "Free address:\t" << mFreeLargeElementsList[i].mAddress << std::endl;
			}
			std::cout << "Elements in list: " << mFreeElementsAllocated << std::endl;

			std::cout << std::endl << std::endl;

			std::cout << "Sizes of saved array elements:" << std::endl;
			for (unsigned int i = 0; i < mSavedArraySizesCount; i++)
			{
				std::cout << "Address:\t" << mSavedArraySizes[i].mMemoryAddress;
				std::cout << "\tSize:\t" << mSavedArraySizes[i].mSize << std::endl;
			}
			std::cout << "Elements in list: " << mSavedArraySizesCount << std::endl;
		}

		std::cout << std::endl << std::endl;
	}

	// -------------------------------------------------------------------------
}