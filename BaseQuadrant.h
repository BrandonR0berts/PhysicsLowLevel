#pragma once

#include "Vector3D.h"
#include "Cube.h"

#include "TimeTracker.h"

#include <thread>

class Quadtree;

class Quadrant abstract
{
public:
	Quadrant(Vec3& minBounds, Vec3& maxBounds, Quadtree& tree);
	virtual ~Quadrant();

	virtual void      AddCube(unsigned int cubeIndex) = 0;
	virtual void      Update(const float deltaTime)   = 0;

	virtual void      CheckCollisions()               = 0;

	        bool      InBounds(Vec3& position);             // In the overall bounds
			bool      WithinShrunkBounds(Vec3& position); // Within the shrunken area

	Vec3            GetMinBounds() const { return mMinBounds; }
	Vec3            GetMaxBounds() const { return mMaxBounds; }

protected:
	Vec3         mMinBounds;
	Vec3         mMaxBounds;

	Quadtree&    mTreePartOf;
};