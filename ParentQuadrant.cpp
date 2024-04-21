#include "ParentQuadrant.h"

#include "LeafQuadrant.h"
#include "Quadtree.h"

#include <iostream>

unsigned int ParentQuadrant::sMaxDepth = 0;

// -----------------------------------------------------------------

ParentQuadrant::ParentQuadrant(unsigned int thisDepth, Vec3& minBounds, Vec3& maxBounds, Quadtree& tree)
	: Quadrant(minBounds, maxBounds, tree)
	, mChildQuadrants()
	, mThisDepth(thisDepth)
	, mLastTime(std::chrono::steady_clock::now())
{
	mLastTime = std::chrono::steady_clock::now();

	// Calculate half the region size so we can pass the right bounds into the child quadrants
	// Does not halve the Y as this is not an octree
	Vec3 halfBoundsExtents    = maxBounds - minBounds;
	     halfBoundsExtents.x /= 2.0f;
	     halfBoundsExtents.z /= 2.0f;

	Vec3         newMinBounds;
	Vec3         newMaxBounds;
	unsigned int column;
	unsigned int row;

	// If the next level down is the max depth, then all of the children need to be leaves
	if (thisDepth + 1 >= sMaxDepth)
	{
		for (unsigned int i = 0; i < 4; i++) 
		{
			column = i % 2;
			row    = i / 2;

			newMinBounds = minBounds + Vec3{ (halfBoundsExtents.x * column),       0.0f, halfBoundsExtents.z * row };
			newMaxBounds = minBounds + Vec3{ (halfBoundsExtents.x * (column + 1)), 0.0f, halfBoundsExtents.z * (row + 1) };

			mChildQuadrants[i] = new LeafQuadrant(newMinBounds, newMaxBounds, tree);
		}
	}
	else // We are not quite at the max depth yet so keep going with parents
	{
		for (unsigned int i = 0; i < 4; i++)
		{
			column = i % 2;
			row    = i / 2;

			newMinBounds = minBounds + Vec3{ (halfBoundsExtents.x * column),       0.0f, halfBoundsExtents.z * row       };
			newMaxBounds = minBounds + Vec3{ (halfBoundsExtents.x * (column + 1)), 0.0f, halfBoundsExtents.z * (row + 1) };

			mChildQuadrants[i] = new ParentQuadrant(thisDepth + 1, newMinBounds, newMaxBounds, tree);
		}
	}
}

// ----------------------------------------------

ParentQuadrant::~ParentQuadrant()
{
	// Clear any neighbours stored so that we dont hit any dangling pointer issues
	if (mThisDepth + 1 == sMaxDepth)
	{
		LeafQuadrant* empty[4] = { nullptr, nullptr, nullptr, nullptr };
		for (unsigned int i = 0; i < 4; i++)
		{
			((LeafQuadrant*)mChildQuadrants[i])->SetNeighbours(empty);
		}
	}

	for (unsigned int i = 0; i < 4; i++)
	{
		if (mChildQuadrants[i])
		{
			delete mChildQuadrants[i];
			mChildQuadrants[i] = nullptr;
		}
	}
}

// ----------------------------------------------

void ParentQuadrant::AddCube(unsigned int cubeIndex)
{
	// Grab the cube being referenced
	Box* cube = mTreePartOf.GetCube(cubeIndex);

	if (!cube)
		return;

	// If the cube is not in the bounds of this parent then it will not be in bounds of any children nodes
	if (!InBounds(cube->position))
		return;

	// All children are leaf nodes
	if (sMaxDepth == mThisDepth)
	{
		Vec3 halfBoundsExtents    = mMaxBounds - mMinBounds;
		     halfBoundsExtents.x /= 2.0f;
		     halfBoundsExtents.z /= 2.0f;

		Vec3 midPoint             = mMinBounds + halfBoundsExtents;

		if (cube->position.x < midPoint.x)
		{
			if (cube->position.z < midPoint.z)
			{
				((LeafQuadrant*)mChildQuadrants[0])->QueueCubeToAdd(cubeIndex);
			}
			else
			{
				((LeafQuadrant*)mChildQuadrants[2])->QueueCubeToAdd(cubeIndex);
			}
		}
		else
		{
			if (cube->position.z < midPoint.z)
			{
				((LeafQuadrant*)mChildQuadrants[1])->QueueCubeToAdd(cubeIndex);
			}
			else
			{
				((LeafQuadrant*)mChildQuadrants[3])->QueueCubeToAdd(cubeIndex);
			}
		}
	}
	else
	{
		// All children are parent nodes
		for (unsigned int i = 0; i < 4; i++)
		{
			if (!mChildQuadrants[i])
				continue;

			if (((ParentQuadrant*)mChildQuadrants[i])->InBounds(cube->position))
			{
				mChildQuadrants[i]->AddCube(cubeIndex);
				return;
			}
		}
	}
}

// ----------------------------------------------

void ParentQuadrant::Update(const float deltaTime)
{
	for (unsigned int i = 0; i < 4; i++)
	{
		if (!mChildQuadrants[i])
			continue;

		mChildQuadrants[i]->Update(deltaTime);
	}
}

// ----------------------------------------------

void ParentQuadrant::CheckCollisions()
{  
	for (unsigned int i = 0; i < 4; i++)
	{
		if (!mChildQuadrants[i])
			continue;

		// Only continue the recursion of the child is a parent as well
		if(mThisDepth < sMaxDepth)
			mChildQuadrants[i]->CheckCollisions();
		else
		{
			// If the collisions are being handled by a seperate thread then we dont want to do it here			
			mChildQuadrants[i]->CheckCollisions();
		}
	}
}

// ----------------------------------------------

LeafQuadrant* ParentQuadrant::FindQuadrantForPosition(Vec3& position)
{
	if (!InBounds(position))
		return nullptr;

	if (mThisDepth + 1 == sMaxDepth)
	{
		Vec3 halfBoundsExtents    = mMaxBounds - mMinBounds;
		     halfBoundsExtents.x /= 2.0f;
		     halfBoundsExtents.z /= 2.0f;

		Vec3 midPoint             = mMinBounds + halfBoundsExtents;

		if (position.x < midPoint.x)
		{
			if (position.z < midPoint.z)
			{
				return (LeafQuadrant*)mChildQuadrants[0];
			}
			else
			{
				return (LeafQuadrant*)mChildQuadrants[2];
			}
		}
		else
		{
			if (position.z < midPoint.z)
			{
				return (LeafQuadrant*)mChildQuadrants[1];
			}
			else
			{
				return (LeafQuadrant*)mChildQuadrants[3];
			}
		}
	}
	else
	{
		// All children are parent nodes
		for (unsigned int i = 0; i < 4; i++)
		{
			if (!mChildQuadrants[i])
				continue;

			ParentQuadrant* quadrant = (ParentQuadrant*)mChildQuadrants[i];

			if (quadrant)
			{
				if (quadrant->InBounds(position))
				{
					return quadrant->FindQuadrantForPosition(position);
				}
			}
		}
	}

	return nullptr;
}

// ----------------------------------------------

ParentQuadrant* ParentQuadrant::FindParentContainingBothLeaves(LeafQuadrant* quadrant1, LeafQuadrant* quadrant2)
{
	// -------------------------------------------------

	bool foundFirstQuadrant  = false;
	bool foundSecondQuadrant = false;

	// If this is just beofre the max depth then this contains leaves
	if (mThisDepth + 1 >= sMaxDepth)
	{
		// See if this contains both leaves
		for (unsigned int i = 0; i < 4; i++)
		{
			if (mChildQuadrants[i] == quadrant1)
				foundFirstQuadrant = true;
			else if (mChildQuadrants[i] == quadrant2)
				foundSecondQuadrant = true;
		}

		if (foundFirstQuadrant && foundSecondQuadrant)
			return this;
	}
	else
	{
		// This is not the max depth, so it contains other parents, so pass down the call
		for (unsigned int i = 0; i < 4; i++)
		{
			ParentQuadrant* quadrant = ((ParentQuadrant*)mChildQuadrants[i])->FindParentContainingBothLeaves(quadrant1, quadrant2);

			// If we have found it then return it back up
			if (quadrant)
			{
				return quadrant;
			}
		}

		// So now we know that no parent themselves contains both, so it must be on a boundary between two 
		for (unsigned int i = 0; i < 4; i++)
		{
			if (!foundFirstQuadrant)
			{
				foundFirstQuadrant = ((ParentQuadrant*)mChildQuadrants[i])->ContainsLeaf(quadrant1);
			}

			if (!foundSecondQuadrant)
			{
				foundSecondQuadrant = ((ParentQuadrant*)mChildQuadrants[i])->ContainsLeaf(quadrant2);
			}
		}

		if (foundFirstQuadrant && foundSecondQuadrant)
		{
			return this;
		}
	}

	return nullptr;
}

// ----------------------------------------------

void ParentQuadrant::CalculateNeighbours()
{
	// First see if we contain leaves
	if (mThisDepth + 1 == sMaxDepth)
	{
		LeafQuadrant* neighbours[4];
		Vec3          minBounds;
		Vec3          maxBounds;
		Vec3          difference;
		Vec3          centre;
		Vec3          checkPoint;

		// Go through each leaf and get their position, then find the leaves that are connecting to that leaf
		for (unsigned int i = 0; i < 4; i++)
		{
			if (mChildQuadrants[i])
			{
				minBounds = mChildQuadrants[i]->GetMinBounds();
				maxBounds = mChildQuadrants[i]->GetMaxBounds();

				// Calculate the centre point through the difference
				difference = maxBounds - minBounds;
				centre     = minBounds + (difference / 2.0f);

				// Offset by set amounts to determine the centre of other leves
				checkPoint = centre + Vec3(difference.x, 0.0f, 0.0f);
				neighbours[0] = mTreePartOf.FindQuadrantContainingPosition(checkPoint);

				checkPoint = centre - Vec3(difference.x, 0.0f, 0.0f);
				neighbours[1] = mTreePartOf.FindQuadrantContainingPosition(checkPoint);

				checkPoint = centre + Vec3(0.0f, 0.0f, difference.z);
				neighbours[2] = mTreePartOf.FindQuadrantContainingPosition(checkPoint);

				checkPoint = centre - Vec3(0.0f, 0.0f, difference.z);
				neighbours[3] = mTreePartOf.FindQuadrantContainingPosition(checkPoint);

				// Now passs the neighbours to the leaf
				((LeafQuadrant*)mChildQuadrants[i])->SetNeighbours(neighbours);
			}
		}
	}
	else
	{
		for (unsigned int i = 0; i < 4; i++)
		{
			if (mChildQuadrants[i])
				((ParentQuadrant*)mChildQuadrants[i])->CalculateNeighbours();
		}
	}
}

// ----------------------------------------------

bool ParentQuadrant::ContainsLeaf(LeafQuadrant* quadrant)
{
	if (mThisDepth + 1 == sMaxDepth)
	{
		for (unsigned int i = 0; i < 4; i++)
		{
			if (mChildQuadrants[i] == quadrant)
			{
				return true;
			}
		}
	}

	return false;
}

// ----------------------------------------------

void ParentQuadrant::AddLeavesToJobList(std::vector<LeafQuadrant*>& leaves)
{
	if (mThisDepth + 1 == sMaxDepth)
	{
		leaves.push_back((LeafQuadrant*)mChildQuadrants[0]);
		leaves.push_back((LeafQuadrant*)mChildQuadrants[1]);
		leaves.push_back((LeafQuadrant*)mChildQuadrants[2]);
		leaves.push_back((LeafQuadrant*)mChildQuadrants[3]);
	}
	else
	{
		if (mChildQuadrants[0] && mChildQuadrants[1] && mChildQuadrants[2] && mChildQuadrants[3])
		{
			((ParentQuadrant*)mChildQuadrants[0])->AddLeavesToJobList(leaves);
			((ParentQuadrant*)mChildQuadrants[1])->AddLeavesToJobList(leaves);
			((ParentQuadrant*)mChildQuadrants[2])->AddLeavesToJobList(leaves);
			((ParentQuadrant*)mChildQuadrants[3])->AddLeavesToJobList(leaves);
		}
	}
}

// ----------------------------------------------