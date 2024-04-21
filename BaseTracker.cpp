#include "BaseTracker.h"

#include "GlobalTrackers.h"
#include <iostream>

#if MemoryOverride

#include <malloc.h>
#include <mutex>

#include "MemoryPool.h"

// ------------------------------------------------------------------------------------------------------ 
// ------------------------------------------------------------------------------------------------------ 
// ------------------------------------------------------------------------------------------------------ 

// Global override of new - is called whenever something calls new and doesn't have its own new override
void* operator new(size_t size) 
{
	std::mutex* mutex = Memory::MemoryPool::Get()->GetMutex();

	if(mutex)
		mutex->lock();

	unsigned int originalDataSize = size;

#if UseMemoryTracking
	// Add the size of the header and the footer to the amount of bytes to allocate
	size += sizeof(Header) + sizeof(Footer);
#endif

	// Now allocate the memory
#if UseMemoryPools
	void* newMemory = Memory::MemoryPool::Get()->AssignMemory(size, false);
#else
	void* newMemory = malloc(size);
#endif


	if (!newMemory)
	{
		if (mutex)
			mutex->unlock();

		return nullptr;
	}
	
#if UseMemoryTracking
	// Setup the header
	Header* header = (Header*)newMemory; 
			*header = Header(size, nullptr, nullptr);

	// Now setup the footer
	Footer* footer = (Footer*)((char*)newMemory + sizeof(Header) + originalDataSize);
			*footer = Footer();

	// Now add the memory for this call to the global generic tracker
	if(Memory::mGenericTracker != nullptr)
	{
		Memory::mGenericTracker->AddMemoryAllocated(header);
	}
#endif

	if (mutex)
		mutex->unlock();

#if UseMemoryTracking
	// Now return back the memory address to the caller so that they can use it as before
	return (void*)((char*)newMemory + sizeof(Header));
#else
	return (void*)newMemory;
#endif
}

// ------------------------------------------------------------------------------------------------------ 

void* operator new[](size_t size)
{
	std::mutex* mutex = Memory::MemoryPool::Get()->GetMutex();

	if (mutex)
		mutex->lock();

	unsigned int originalDataSize = size;

#if UseMemoryTracking
	// Add the size of the header and the footer to the amount of bytes to allocate
	size += sizeof(Header) + sizeof(Footer);
#endif

	// Now allocate the memory
#if UseMemoryPools
	void* newMemory = Memory::MemoryPool::Get()->AssignMemory(size, true);
#else
	void* newMemory = malloc(size);
#endif

	if (!newMemory)
	{
		if (mutex)
			mutex->unlock();

		return nullptr;
	}
	
#if UseMemoryTracking
	// Setup the header
	Header* header = (Header*)newMemory; 
			*header = Header(size, nullptr, nullptr);

	// Now setup the footer
	Footer* footer = (Footer*)((char*)newMemory + sizeof(Header) + originalDataSize);
			*footer = Footer();

	// Now add the memory for this call to the global generic tracker
	if(Memory::mGenericTracker != nullptr)
	{
		Memory::mGenericTracker->AddMemoryAllocated(header);
	}
#endif

	if (mutex)
		mutex->unlock();

#if UseMemoryTracking
	// Now return back the memory address to the caller so that they can use it as before
	return (void*)((char*)newMemory + sizeof(Header));
#else
	return (void*)newMemory;
#endif
}

// ------------------------------------------------------------------------------------------------------ 
// ------------------------------------------------------------------------------------------------------ 
// ------------------------------------------------------------------------------------------------------ 

void operator delete(void* pointer, size_t size)
{
	if (!pointer)
		return;

	std::mutex* mutex = Memory::MemoryPool::Get()->GetMutex();

	if (mutex)
		mutex->lock();

	void* startOfMemory = pointer;

#if UseMemoryTracking
	// Jump back to the start of the header and then delete all of the memory allocated
	startOfMemory = (void*)((char*)pointer - sizeof(Header));

	if (Memory::mGenericTracker != nullptr)
	{
		Memory::mGenericTracker->RemoveMemoryAllocated((Header*)startOfMemory);
	}
#endif

#if UseMemoryPools
	Memory::MemoryPool::Get()->FreeMemory(size, startOfMemory, false);
#else
	// Free the memory now we are in the right place
	free(startOfMemory);
#endif

	if (mutex)
		mutex->unlock();
}

// ------------------------------------------------------------------------------------------------------ 

void operator delete[](void* pointer)
{
	if (!pointer)
		return;

	std::mutex* mutex = Memory::MemoryPool::Get()->GetMutex();

	if (mutex)
		mutex->lock();

	void* startOfMemory = pointer;

#if UseMemoryTracking
	// Jump back to the start of the header and then delete all of the memory allocated
	startOfMemory = (void*)((char*)pointer - sizeof(Header));

	if (Memory::mGenericTracker != nullptr)
	{
		Memory::mGenericTracker->RemoveMemoryAllocated((Header*)startOfMemory);
	}
#endif

#if UseMemoryPools
	Memory::MemoryPool::Get()->FreeMemory(0, startOfMemory, true);
#else
	// Free the memory now we are in the right place
	free(startOfMemory);
#endif

	if (mutex)
		mutex->unlock();
}

// ------------------------------------------------------------------------------------------------------ 
// ------------------------------------------------------------------------------------------------------ 
// ------------------------------------------------------------------------------------------------------ 

#endif

BaseTracker::BaseTracker()
	: mBytesAllocated(0)
	, mFirstMemoryAddress(nullptr)
	, mLastMemoryAddress(nullptr)
{


}

// ------------------------------------------------------------------------------------------------------ 

BaseTracker::BaseTracker(Header* firstMemoryAddress)
	: mBytesAllocated(0)
	, mFirstMemoryAddress(firstMemoryAddress)
	, mLastMemoryAddress(firstMemoryAddress)
{

}

// ------------------------------------------------------------------------------------------------------ 

BaseTracker::~BaseTracker()
{

}

// ------------------------------------------------------------------------------------------------------ 

bool BaseTracker::WalkTheHeap()
{
	if (!mFirstMemoryAddress)
		return false;

	bool allChecksPassed = true;

	Header* currentAddress = mFirstMemoryAddress;

	unsigned int headerAndFooterSize = sizeof(Header) + sizeof(Footer);

	while (currentAddress != nullptr)
	{
		// Output the memory address
		std::cout << "Memory Address:\t\t" << currentAddress << std::endl;

		// Data size
		std::cout << "\t\t--Data size:\t\t" << currentAddress->mBytesInMemory - headerAndFooterSize << "(+" << headerAndFooterSize << ")" << std::endl;

		// Next address
		std::cout << "\t\t--Next header:\t\t" << currentAddress->mNextHeader << std::endl;

		// Prior header
		std::cout << "\t\t--Prior header:\t\t" << currentAddress->mPriorHeader << std::endl;

		// Checksum check
		bool checkPassed = currentAddress->mCheckValue == ChecksumValue;
		std::cout << "\t\t--Checksum valid:\t" << (checkPassed ? "yes" : "no") << std::endl;

		if (!checkPassed)
			allChecksPassed = false;

		currentAddress = currentAddress->mNextHeader;
	}

	return allChecksPassed;
}

// ------------------------------------------------------------------------------------------------------ 

void BaseTracker::AddMemoryAllocated(Header* header)
{
	if (!header)
		return;

	if (header->mCheckValue != ChecksumValue)
	{
		std::cout << "a";
	}

	mBytesAllocated += header->mBytesInMemory;

	if (mFirstMemoryAddress == nullptr)
	{
		mFirstMemoryAddress = header;
		mLastMemoryAddress  = header;
	}
	else if(mLastMemoryAddress)
	{
		mLastMemoryAddress->mNextHeader = header;

		header->mPriorHeader = mLastMemoryAddress;

		mLastMemoryAddress = header;
	}
}

// ------------------------------------------------------------------------------------------------------ 

void BaseTracker::RemoveMemoryAllocated(Header* header)
{
	if (!header)
		return;

	mBytesAllocated -= header->mBytesInMemory;

	if (mFirstMemoryAddress == header)
	{
		if (mFirstMemoryAddress->mNextHeader)
			mFirstMemoryAddress->mNextHeader->mPriorHeader = nullptr;

		mFirstMemoryAddress = header->mNextHeader;
	}
	else if (mLastMemoryAddress == header)
	{
		if (mLastMemoryAddress->mPriorHeader)
		{
			mLastMemoryAddress              = mLastMemoryAddress->mPriorHeader;
			mLastMemoryAddress->mNextHeader = nullptr;
		}
	}
	else
	{
		if (header->mNextHeader && header->mPriorHeader)
		{
			header->mNextHeader->mPriorHeader = header->mPriorHeader;
			header->mPriorHeader->mNextHeader = header->mNextHeader;
		}
	}
}

// ------------------------------------------------------------------------------------------------------ 