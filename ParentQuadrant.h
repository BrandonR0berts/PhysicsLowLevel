#include "BaseQuadrant.h"

#include "Commons.h"

#include <mutex>

class LeafQuadrant;

class ParentQuadrant final : public Quadrant
{
public:
	ParentQuadrant(unsigned int thisDepth, Vec3& minBounds, Vec3& maxBounds, Quadtree& tree);
	~ParentQuadrant() override;

	void            AddCube(unsigned int cubeIndex)         override;
	void            Update(const float deltaTime)           override;

	void            CheckCollisions()                       override;
	LeafQuadrant*   FindQuadrantForPosition(Vec3& position);

	void            AddLeavesToJobList(std::vector<LeafQuadrant*>& leaves);

	ParentQuadrant* FindParentContainingBothLeaves(LeafQuadrant* quadrant1, LeafQuadrant* quadrant2);
	bool            ContainsLeaf(LeafQuadrant* quadrant);

	void            CalculateNeighbours();

	static unsigned int sMaxDepth;

private:
	Quadrant*                             mChildQuadrants[4];
	unsigned int                          mThisDepth;
	std::chrono::steady_clock::time_point mLastTime;
};