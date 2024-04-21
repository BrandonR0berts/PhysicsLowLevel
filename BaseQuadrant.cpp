#include "BaseQuadrant.h"

#include "Quadtree.h"
#include "TimeTracker.h"

Quadrant::Quadrant(Vec3& minBounds, Vec3& maxBounds, Quadtree& tree)
	: mMinBounds(minBounds)
	, mMaxBounds(maxBounds)
	, mTreePartOf(tree)
{
	
}

// ----------------------------------------------

Quadrant::~Quadrant()
{

}

// ----------------------------------------------

bool Quadrant::InBounds(Vec3& position)
{
	if (position.x < mMinBounds.x || position.x > mMaxBounds.x)
		return false;

	if (position.z < mMinBounds.z || position.z > mMaxBounds.z)
		return false;

	return true;
}

// ----------------------------------------------

bool Quadrant::WithinShrunkBounds(Vec3& position)
{
	if (position.x < mMinBounds.x + SegmentBounarySize || position.x > mMaxBounds.x - SegmentBounarySize)
		return false;

	if (position.z < mMinBounds.z + SegmentBounarySize || position.z > mMaxBounds.z - SegmentBounarySize)
		return false;

	return true;
}

// ---------------------------------------------