#pragma once

#include "Vector3D.h"
#include "BaseQuadrant.h"

#include "Commons.h"
#include "TimeTracker.h"

#include <vector>
#include <mutex>

class LeafQuadrant final : public Quadrant
{
public:
	LeafQuadrant(Vec3& minBounds, Vec3& maxBounds, Quadtree& tree);
	~LeafQuadrant() override;

	void        AddCube(unsigned int cubeIndex) override;
	void        Update(const float deltaTime)   override;
	void        ThreadUpdate(const float deltaTime);
	void        CheckCollisions()               override;

	void        AddCubeToBoundaries(unsigned int cubeID);

	bool        QueueCubeToAdd(unsigned int cubeIndex);

	void        SetNeighbours(LeafQuadrant* neighbours[4]);

	//std::vector<int>                  GetBoundaryCubes() { return mCubesInBoundry; }
	//std::vector<std::pair<int, bool>> GetSegmentCubes()  { return mCubesInSegment; }

	std::vector<int>&                  GetBoundaryCubes() { return mCubesInBoundry; }
	std::vector<std::pair<int, bool>>& GetSegmentCubes()  { return mCubesInSegment; }

	std::mutex& GetModifyingMutex() { return mModifyingMutex; }

private:
	bool HandleBorderCube(Box* cube, unsigned int cubeID, unsigned int internalID);
	void HandleSegmentCube(Box* cube, unsigned int cubeID, unsigned int internalID);

	void AddPendingCubes();
	void UpdatePhysics(const float deltaTime);
	void RemoveCubesMarked();
	void HandleCubesTransitioning();

	std::vector<std::pair<int, bool>>          mCubesInSegment; // ID points to a cube in the tree's list - second says if it is in the boundary or not
	std::vector<int>                           mCubesInBoundry; // ID points to an index in the mCubesInSegment list

	std::pair<unsigned int, bool>              mCubesToAddToQuadrant[MaxCubeTransferRate];      // ID points to a cube in the tree's list
	unsigned int                               mCubesToAddToQuadrantFillCount;                  // The amount of elements in the array on the line above

	unsigned int                               mCubeIDsMovedOutOfQuadrant[MaxCubeTransferRate]; // ID is an index into the mCubesInBoundary vector
	unsigned int                               mCubesMovedOutOfQuadrentIndex;                   // The amount of elements in the array on the line above

	LeafQuadrant*                              mNeighbours[4];
	std::mutex                                 mQueuedCubeBlockingMutex; // Mutex so that external calls cannot add cubes while we are clearing them up/adding them
	std::mutex                                 mModifyingMutex;          // Mutex so that external calls cannot copy a list while it is in an invalid state
};