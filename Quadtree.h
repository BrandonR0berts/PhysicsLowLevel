#pragma once

#include "Cube.h"
#include "Vector3D.h"
#include "Commons.h"
#include "TimeTracker.h"

#include <vector>
#include <mutex>
#include <thread>

#include <functional>

// -------------------------------------

class Quadrant;
class LeafQuadrant;

// -------------------------------------

class Quadtree
{
public:
	Quadtree(unsigned int depth, Vec3 minBounds, Vec3 maxBounds);
	~Quadtree();

	void AddCubeToTree(Box cube);
	void Update(const float deltaTime);
	void Render();
	void ImpulseAllBoxes(float amount);
	void CheckCollisions();

	bool QueueAddCubeToTree(unsigned int cubeIndex);

	void               ThreadJobGetter();

	bool               GetProgramRunning() const { return mProgramRunning; }

	Box*               GetCube(unsigned int index);

	LeafQuadrant*      FindQuadrantContainingPosition(Vec3& position);

	TimeTracker& GetTimeTracker() { return mUpdateTimeTracker; }

private:
	std::vector<Box>          mCubes;

	Quadrant*                 mBaseQuadrant;
	unsigned int              mTreeDepth;

	bool                       mProgramRunning;

	std::vector<LeafQuadrant*> mLeafJobs;
	std::vector<LeafQuadrant*> mJobCopy;
	std::vector<LeafQuadrant*> mJobsBeingProcessed;

	std::vector<std::thread*>  mThreads;

	std::mutex                 mJobBlockerMutex;
	float                      mDeltaTimeStore;

	std::condition_variable    mWaitForJobsDone;
	std::mutex                 mJobDoneWaitMutex;

	TimeTracker                mUpdateTimeTracker;

	std::atomic<int>           mThreadsWaiting;
};

// -------------------------------------