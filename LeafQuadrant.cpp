#include "LeafQuadrant.h"
#include "ParentQuadrant.h"

#include "Quadtree.h"

#include <iostream>

// ----------------------------------------------

LeafQuadrant::LeafQuadrant(Vec3& minBounds, Vec3& maxBounds, Quadtree& tree)
	: Quadrant(minBounds, maxBounds, tree)
	, mCubesInSegment()
	, mCubesInBoundry()
	, mCubesToAddToQuadrant()
	, mCubesToAddToQuadrantFillCount(0)
	, mCubeIDsMovedOutOfQuadrant()
	, mCubesMovedOutOfQuadrentIndex(0)
	, mNeighbours {nullptr, nullptr, nullptr, nullptr}
	, mQueuedCubeBlockingMutex()
	, mModifyingMutex()
{

}

// ----------------------------------------------

LeafQuadrant::~LeafQuadrant()
{
	for (unsigned int i = 0; i < 4; i++)
	{
		mNeighbours[i] = nullptr;
	}
}

// ----------------------------------------------

void LeafQuadrant::AddCube(unsigned int cubeIndex)
{
	QueueCubeToAdd(cubeIndex);
}

// ----------------------------------------------

bool LeafQuadrant::QueueCubeToAdd(unsigned int cubeIndex)
{
	if (mCubesToAddToQuadrantFillCount >= MaxCubeTransferRate)
		return false;

	mQueuedCubeBlockingMutex.lock();
		mCubesToAddToQuadrant[mCubesToAddToQuadrantFillCount] = { cubeIndex, false };
		mCubesToAddToQuadrantFillCount++;
	mQueuedCubeBlockingMutex.unlock();

	return true;
}

// ----------------------------------------------

void LeafQuadrant::AddCubeToBoundaries(unsigned int cubeID)
{
	if (mCubesToAddToQuadrantFillCount >= MaxCubeTransferRate)
		return;

	mQueuedCubeBlockingMutex.lock();
		mCubesToAddToQuadrant[mCubesToAddToQuadrantFillCount] = { cubeID, true };
		mCubesToAddToQuadrantFillCount++;
	mQueuedCubeBlockingMutex.unlock();
}

// ----------------------------------------------

void LeafQuadrant::HandleSegmentCube(Box* cube, unsigned int cubeID, unsigned int internalID)
{
	if (!cube || mCubesInSegment[internalID].second)
		return;
	
	Vec3 cubePosition = cube->position;

	// See if this cube has gone into the border or beyond then we need to pass the cube into the border section
	if (!WithinShrunkBounds(cubePosition))
	{
		// See if the cube has gone right out of the segment
		if (!InBounds(cubePosition))
		{
			// See if we have any spaces left for cubes to be removed
			if (mCubesMovedOutOfQuadrentIndex >= MaxCubeTransferRate)
				return;

			bool goneIntoNeighbour = false;

			// Not in this quadrent, so find where it has gone
			for (unsigned int i = 0; i < 4; i++)
			{				
				if (mNeighbours[i] && mNeighbours[i]->InBounds(cubePosition))
				{
					goneIntoNeighbour = true;

					if (!mNeighbours[i]->WithinShrunkBounds(cubePosition))
					{
						mNeighbours[i]->AddCubeToBoundaries(cubeID);

#if VisualiseQuadtree
						cube->colour = Vec3(1.0f, 1.0f, 1.0f);
#endif
					}
					else
					{
						mNeighbours[i]->AddCube(cubeID);

#if VisualiseQuadtree
						cube->colour = Vec3(1.0f, 0.0f, 0.0f);
#endif
					}

					// As this cube was not in the boundary, we need to add it so that when the removal process removes it it has something to run
					mModifyingMutex.lock();
						mCubesInBoundry.push_back(internalID);

						mCubeIDsMovedOutOfQuadrant[mCubesMovedOutOfQuadrentIndex] = mCubesInBoundry.size() - 1;
						mCubesMovedOutOfQuadrentIndex++;

					mModifyingMutex.unlock();

					return;
				}
			}

			if (!goneIntoNeighbour)
			{
				// The cube must have gone too far in a diagonal to be in a neighbour now
				LeafQuadrant* goneInto = mTreePartOf.FindQuadrantContainingPosition(cubePosition);

				if (!goneInto)
					return;

				if (!goneInto->WithinShrunkBounds(cubePosition))
				{
					goneInto->AddCubeToBoundaries(cubeID);

#if VisualiseQuadtree
					cube->colour = Vec3(1.0f, 1.0f, 1.0f);
#endif
				}
				else
				{
					goneInto->AddCube(cubeID);

#if VisualiseQuadtree
					cube->colour = Vec3(1.0f, 0.0f, 0.0f);
#endif
				}

				// As this cube was not in the boundary, we need to add it so that when the removal process removes it it has something to run
				mModifyingMutex.lock();
					mCubesInBoundry.push_back(internalID);

					mCubeIDsMovedOutOfQuadrant[mCubesMovedOutOfQuadrentIndex] = mCubesInBoundry.size() - 1;
					mCubesMovedOutOfQuadrentIndex++;

				mModifyingMutex.unlock();
			}

			return;
		}

		// The cube is now still in the quadrent, but in the border
		mModifyingMutex.lock();
			mCubesInBoundry.push_back(internalID);
		mModifyingMutex.unlock();

		// Set that this cube is in the border
		mCubesInSegment[internalID].second = true;

#if VisualiseQuadtree
		cube->colour = Vec3(1.0f, 1.0f, 1.0f);
#endif
	}
}

// ----------------------------------------------

bool LeafQuadrant::HandleBorderCube(Box* cube, unsigned int cubeID, unsigned int internalID)
{
	if (!cube || !mCubesInSegment[mCubesInBoundry[internalID]].second)
		return false;

	Vec3 cubePosition = cube->position;

	// If gone into a different leaf
	if (!InBounds(cubePosition))
	{
		if (mCubesMovedOutOfQuadrentIndex >= MaxCubeTransferRate)
			return false;

		// Find where it has gone to and add it there
		for (unsigned int i = 0; i < 4; i++)
		{		
			if (mNeighbours[i] && mNeighbours[i]->InBounds(cubePosition))
			{
				if (!mNeighbours[i]->WithinShrunkBounds(cubePosition))
				{
					mNeighbours[i]->AddCubeToBoundaries(cubeID);

#if VisualiseQuadtree
					cube->colour = Vec3(1.0f, 1.0f, 1.0f);
#endif
				}
				else
				{
					mNeighbours[i]->AddCube(cubeID);

#if VisualiseQuadtree
					cube->colour = Vec3(1.0f, 0.0f, 0.0f);
#endif
				}

				// Now we have added it to the other segment, we need to mark this for delete
				mCubeIDsMovedOutOfQuadrant[mCubesMovedOutOfQuadrentIndex] = internalID;
				mCubesMovedOutOfQuadrentIndex++;

				return false;
			}
		}
	}
	else if (WithinShrunkBounds(cubePosition))
	{

		// Moved back into the normal section of the leaf, so need to remove it from the internal lists - and mark it as not in a boundary
		mCubesInSegment[mCubesInBoundry[internalID]].second = false;

#if VisualiseQuadtree
		cube->colour = Vec3(1.0f, 0.0f, 0.0f);
#endif

		return true;
	}

	return false;
}

// ----------------------------------------------

void LeafQuadrant::AddPendingCubes()
{
	mQueuedCubeBlockingMutex.lock();

		// First add the boxes that are queued up to be added to this quadrant
		for (unsigned int i = 0; i < mCubesToAddToQuadrantFillCount; i++)
		{
			mCubesInSegment.push_back(mCubesToAddToQuadrant[i]);

			// If the cube is being added to the border, then also add one to that list
			if (mCubesToAddToQuadrant[i].second)
			{
				mCubesInBoundry.push_back(mCubesInSegment.size() - 1);
			}
		}
		mCubesToAddToQuadrantFillCount = 0;

	mQueuedCubeBlockingMutex.unlock();
}

// ----------------------------------------------

void LeafQuadrant::UpdatePhysics(const float deltaTime)
{
	// Only this list gets update called as it contains the cubes in the boundary
	unsigned int cubeCount = (unsigned int)mCubesInSegment.size();
	for (unsigned int i = 0; i < cubeCount; i++)
	{
		Box* cube = mTreePartOf.GetCube(mCubesInSegment[i].first);

		if (!cube)
			continue;

		cube->UpdatePhysics(deltaTime);
	}
}

// ----------------------------------------------

void LeafQuadrant::RemoveCubesMarked()
{
	if (mCubesMovedOutOfQuadrentIndex == 0)
		return;

	mModifyingMutex.lock();

		// Now handkle cubes that have moved out of the segment
		for (unsigned int i = 0; i < mCubesMovedOutOfQuadrentIndex; i++)
		{
			// Grab the indexes to remove
			int indexInBoundaryList = mCubeIDsMovedOutOfQuadrant[i];
			int indexInSegmentList  = mCubesInBoundry[indexInBoundaryList];

			mCubesInBoundry[indexInBoundaryList]      = -1;
			mCubesInSegment[indexInSegmentList].first = -1;
		}

		// Now go through and remove any that are marked as -1
		unsigned int borderCubeCount = (int)mCubesInBoundry.size();
		for (unsigned int i = 0; i < borderCubeCount; i++)
		{
			if (mCubesInBoundry[i] < 0)
			{
				mCubesInBoundry.erase(mCubesInBoundry.begin() + i);

				i--;
				borderCubeCount--;
			}
		}

		int segmentCubeCount = (unsigned int)mCubesInSegment.size();
		for (int i = 0; i < segmentCubeCount; i++)
		{
			if (mCubesInSegment[i].first < 0)
			{
				// ---------------------------------------------

				// Make sure that the indexes that the border cubes are pointing to are the correct index - may need to reduce the value by 1
				for (unsigned int j = 0; j < borderCubeCount; j++)
				{
					if (mCubesInBoundry[j] > i)
					{
						mCubesInBoundry[j]--;
					}
				}

				// ---------------------------------------------

				mCubesInSegment.erase(mCubesInSegment.begin() + i);

				i--;
				segmentCubeCount--;

				// ---------------------------------------------
			}
		}
		mCubesMovedOutOfQuadrentIndex = 0;

	mModifyingMutex.unlock();
}

// ----------------------------------------------

void LeafQuadrant::HandleCubesTransitioning()
{
	unsigned int cubeCount = (unsigned int)mCubesInSegment.size();
	for (unsigned int i = 0; i < cubeCount; i++)
	{
		Box* cube = mTreePartOf.GetCube(mCubesInSegment[i].first);

		if (!cube)
			continue;

		HandleSegmentCube(cube, mCubesInSegment[i].first, i);
	}

	cubeCount = (unsigned int)mCubesInBoundry.size();

	// Lock the mutex as we are going to be making the data into an invalid state
	mModifyingMutex.lock();

		bool changed = false;

		for (unsigned int i = 0; i < cubeCount; i++)
		{
			Box* cube = mTreePartOf.GetCube(mCubesInSegment[mCubesInBoundry[i]].first);

			if (!cube)
				continue;

			if(HandleBorderCube(cube, mCubesInSegment[mCubesInBoundry[i]].first, i))
			{
				changed = true;

				// mark the cube to be removed from the list
				mCubesInBoundry[i] = -1;
			}
		}

		if (!changed)
		{
			mModifyingMutex.unlock();
			return;
		}

		for (unsigned int i = 0; i < cubeCount; i++)
		{
			// If we need to remove this value
			if (mCubesInBoundry[i] < 0)
			{
				// Remove it
				mCubesInBoundry.erase(mCubesInBoundry.begin() + i);
				
				// See if we need to drop the index of any cubes marked for removal to stay in bounds
				for (unsigned int j = 0; j < mCubesMovedOutOfQuadrentIndex; j++)
				{
					if (mCubeIDsMovedOutOfQuadrant[j] >= i)
					{
						mCubeIDsMovedOutOfQuadrant[j]--;
					}
				}

				i--;
				cubeCount--;
			}
		}

	// Now in valid state so can unlock the mutex
	mModifyingMutex.unlock();
}

// ----------------------------------------------

void LeafQuadrant::Update(const float deltaTime)
{
	AddPendingCubes();

	UpdatePhysics(deltaTime);

	HandleCubesTransitioning();

	RemoveCubesMarked();
}

// ----------------------------------------------

void LeafQuadrant::SetNeighbours(LeafQuadrant* neighbours[4])
{
	for (unsigned int i = 0; i < 4; i++)
	{
		mNeighbours[i] = neighbours[i];
	}
}

// ----------------------------------------------

void LeafQuadrant::ThreadUpdate(const float deltaTime)
{
	// Now call update
	Update(deltaTime);

	// Now check collisions in this quadrant
	CheckCollisions();
}

// ----------------------------------------------

void LeafQuadrant::CheckCollisions()
{
	Box* cubeOne = nullptr;
	Box* cubeTwo = nullptr;

	unsigned int cubeCount     = (unsigned int)mCubesInSegment.size();
	unsigned int boundaryCount = (unsigned int)mCubesInBoundry.size();

	// Collisions within this segment - this includes all of the boundary cubes
	for (unsigned int i = 0; i < cubeCount; i++)
	{
		// Grab the cube
		cubeOne = mTreePartOf.GetCube(mCubesInSegment[i].first);

		if (!cubeOne)
			continue;

		// Go through each other cube
		for (unsigned int j = 0; j < cubeCount; j++)
		{
			if (i == j)
				continue;

			cubeTwo = mTreePartOf.GetCube(mCubesInSegment[j].first);

			if (!cubeTwo)
				continue;

			// Check for a collision
			if (cubeOne->CheckCollision(*cubeTwo))
			{
				// Handle the collision if it does happen
				Box::ResolveCollision(*cubeOne, *cubeTwo);
				break;
			}
		}
	}

	std::vector<int>                  neighbourBounaryCubes;
	std::vector<std::pair<int, bool>> neighbourSegmentCubes;

	// Now check the boundary cubes against the neighbour's boundary cubes
	for (unsigned int i = 0; i < boundaryCount; i++)
	{
		Box* cube = mTreePartOf.GetCube(mCubesInSegment[mCubesInBoundry[i]].first);

		if (!cube)
			continue;

		for (unsigned int j = 0; j < 4; j++)
		{
			if (mNeighbours[j])
			{
				// Lock the mutex to make sure that the data will not be in an invalid state while we copy it
				std::mutex& neighbourMutex = mNeighbours[j]->GetModifyingMutex();

				neighbourMutex.lock();

					neighbourBounaryCubes = mNeighbours[j]->GetBoundaryCubes();
					neighbourSegmentCubes = mNeighbours[j]->GetSegmentCubes();

					unsigned int boundaryCubeCount = (unsigned int)neighbourBounaryCubes.size();

				neighbourMutex.unlock();

				if (neighbourSegmentCubes.empty() || neighbourBounaryCubes.empty())
					continue;

				for (unsigned int k = 0; k < boundaryCubeCount; k++)
				{
					if (k >= neighbourBounaryCubes.size() || neighbourBounaryCubes[k] < 0 || neighbourBounaryCubes[k] >= (int)neighbourSegmentCubes.size())
						continue;

					Box* cubeTwo = mTreePartOf.GetCube(neighbourSegmentCubes[neighbourBounaryCubes[k]].first);

					if (!cubeTwo)
						continue;

					if (cube->CheckCollision(*cubeTwo))
					{
						// Handle the collision if it does happen
						Box::ResolveCollision(*cube, *cubeTwo);

						break;
					}
				}

				// --------------------------------------- //
			}
		}
	}
}

// ----------------------------------------------