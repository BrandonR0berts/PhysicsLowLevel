#include "Quadtree.h"

#include "ParentQuadrant.h"
#include "LeafQuadrant.h"
#include "BaseQuadrant.h"

#include <iostream>

// ----------------------------------------------
// ----------------------------------------------
// ----------------------------------------------

Quadtree::Quadtree(unsigned int depth, Vec3 minBounds, Vec3 maxBounds)
	: mCubes()
	, mBaseQuadrant(nullptr)
	, mTreeDepth(depth)

#if SyncThreads == true
	, mThreadsHitCollisionsBlock(0)
	, mThreadsPassedCollisionBlock(0)

	, mCollisionBlocked(true)
	, mNextFrameBlocked(false)
#endif

	, mProgramRunning(true)

	, mLeafJobs()
	, mThreads()

	, mUpdateTimeTracker()
	, mDeltaTimeStore(0.0f)
	, mJobBlockerMutex()

	, mThreadsWaiting(0)
{
	mCubes.reserve(NUMBER_OF_BOXES);

	// Set the max depth
	ParentQuadrant::sMaxDepth = depth;

	LeafQuadrantCount = (int)std::pow(4, QuadtreeDepth);

	// ------------------------------------

	// If the depth is 0 then we are not breaking the space up at all
	if (depth == 0)
	{
		mBaseQuadrant = new LeafQuadrant(minBounds, maxBounds, *this);
	}
	else
	{
		mBaseQuadrant = new ParentQuadrant(0, minBounds, maxBounds, *this);

		// Now the tree has been created, we need to determine which leaves are neighbours to each other
		((ParentQuadrant*)mBaseQuadrant)->CalculateNeighbours();
	}

	// ------------------------------------	

	// Now populate the job list
	if (depth == 0)
		mLeafJobs.push_back((LeafQuadrant*)mBaseQuadrant);
	else
	{
		((ParentQuadrant*)mBaseQuadrant)->AddLeavesToJobList(mLeafJobs);
	}

	// ------------------------------------	

	for (unsigned int i = 0; i < ThreadsToAllocateToProgram; i++)
	{
		mThreads.push_back(new std::thread(&Quadtree::ThreadJobGetter, this));
	}
}

// ----------------------------------------------

Quadtree::~Quadtree()
{
	// Set the program is over so all threads end
	mProgramRunning = false;

	for (unsigned int i = 0; i < ThreadsToAllocateToProgram; i++)
	{
		if (mThreads[i])
		{
			mThreads[i]->join();

			delete mThreads[i];
			mThreads[i] = nullptr;
		}
	}

	// Now clean up the quadrants
	if (mBaseQuadrant)
	{
		delete mBaseQuadrant;
		mBaseQuadrant = nullptr;
	}
}

// ----------------------------------------------

void Quadtree::AddCubeToTree(Box cube)
{
	// Add the cube to the list
	mCubes.emplace_back(cube);

	if (!mBaseQuadrant)
		return;

	// Now add the cube to the quadrants
	mBaseQuadrant->AddCube(mCubes.size() - 1);
}

// ----------------------------------------------

void Quadtree::Update(const float deltaTime)
{
	if (!mBaseQuadrant)
		return;

	mUpdateTimeTracker.StartTiming();

		mThreadsWaiting = 0;

		mJobCopy        = mLeafJobs;
		mDeltaTimeStore = deltaTime;

		std::unique_lock block(mJobDoneWaitMutex);
		mWaitForJobsDone.wait(block);

	mUpdateTimeTracker.AddMeasurement();
}

// ----------------------------------------------

void Quadtree::ThreadJobGetter()
{
	LeafQuadrant* job = nullptr;

	while (mProgramRunning)
	{		
		mJobBlockerMutex.lock();

			if (mJobCopy.size() > 0)
			{
				// Grab a job to do
				job = mJobCopy[0];
				mJobCopy.erase(mJobCopy.begin());

				mJobsBeingProcessed.push_back(job);
			}

		mJobBlockerMutex.unlock();

		if (job)
		{
			job->ThreadUpdate(mDeltaTimeStore);

			mJobBlockerMutex.lock();

				// Remove the job from the list
				unsigned int jobsBeingProcessedCount = mJobsBeingProcessed.size();
				for (unsigned int i = 0; i < jobsBeingProcessedCount; i++)
				{
					if (mJobsBeingProcessed[i] == job)
					{
						mJobsBeingProcessed.erase(mJobsBeingProcessed.begin() + i);
						break;
					}
				}

			mJobBlockerMutex.unlock();

			job = nullptr;
		}		

		mJobBlockerMutex.lock();
			if (mJobCopy.empty() && mJobsBeingProcessed.empty())
			{
				mWaitForJobsDone.notify_all();
			}
		mJobBlockerMutex.unlock();
	}
}

// ----------------------------------------------

void Quadtree::Render()
{
	unsigned int cubeCount = (unsigned int)mCubes.size();

	for (unsigned int i = 0; i < cubeCount; i++)
	{
		mCubes[i].Render();
	}
}

// ----------------------------------------------

void Quadtree::ImpulseAllBoxes(float amount)
{
	unsigned int cubeCount = (unsigned int)mCubes.size();

	for (unsigned int i = 0; i < cubeCount; i++)
	{
		mCubes[i].velocity.y += amount;
	}
}

// ----------------------------------------------

void Quadtree::CheckCollisions()
{
	if (!mBaseQuadrant)
		return;
	
	// Only run this check if we know that it is a leaf quadrant
	mBaseQuadrant->CheckCollisions();
}

// ----------------------------------------------

bool Quadtree::QueueAddCubeToTree(unsigned int cubeIndex)
{
	if (!mBaseQuadrant)
		return false;

	LeafQuadrant* quadrantToAddTo = nullptr;

	// If the tree depth is 0 then we cannot have a cube go out of bounds of the quadrant, as the quadrant is the whole area
	if (mTreeDepth == 0)
		return false;
		
	Box* cube = GetCube(cubeIndex);

	if (!cube)
		return false;

	Vec3 normalisedDirection = cube->velocity.normalised();
	Vec3 checkPosition       = cube->position + Vec3(normalisedDirection.x * 0.5f, 0.0f, normalisedDirection.z * 0.5f);

	     quadrantToAddTo     = ((ParentQuadrant*)mBaseQuadrant)->FindQuadrantForPosition(checkPosition);
	
	if (!quadrantToAddTo)
		return false;

	return quadrantToAddTo->QueueCubeToAdd(cubeIndex);
}

// ----------------------------------------------

LeafQuadrant* Quadtree::FindQuadrantContainingPosition(Vec3& position)
{
	LeafQuadrant* quadrant = ((ParentQuadrant*)mBaseQuadrant)->FindQuadrantForPosition(position);

	return quadrant;
}

// ----------------------------------------------

Box* Quadtree::GetCube(unsigned int index)
{
	if (mCubes.size() > index)
	{
		return &mCubes[index];
	}

	return nullptr;
}

// ----------------------------------------------