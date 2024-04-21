#pragma once

// Total box count
#define NUMBER_OF_BOXES 50000

// Depth of quad-tree (depth of 0 = all on one layer, depth of 1 = splits area into 4)
#define QuadtreeDepth 4

// Thread count
#define ThreadsToAllocateToProgram 6

// toggle if we are replacing the new/delete functions with our own - note the memory pools wont work if this is false
#define MemoryOverride true

// Swapping out malloc/free with our own memory pools
#define UseMemoryPools true

// Memory tagging - adding a header and footer to the memory we allocate
#define UseMemoryTracking false

// The value checked when walking the heap
#define ChecksumValue 0xAAAABBBB

// Used to show, using colours, where the quadtree's bounds are
#define VisualiseQuadtree false

// The dimensions of the cubes - everything scales according to this value
#define CubeSize 0.1f

static int LeafQuadrantCount = 0;

#define debugCamera false

#if debugCamera == true
	#define LOOKAT_X 10
	#define LOOKAT_Y 35
	#define LOOKAT_Z 50

	#define LOOKDIR_X 10
	#define LOOKDIR_Y -1
	#define LOOKDIR_Z 0
#else
	#define LOOKAT_X 10
	#define LOOKAT_Y 10
	#define LOOKAT_Z 50

	#define LOOKDIR_X 10
	#define LOOKDIR_Y 0
	#define LOOKDIR_Z 0
#endif

#define minX -10.0f
#define maxX  30.0f
#define minZ -30.0f
#define maxZ  30.0f

#define FINE_TUNED_MEASUREMENTS true

#define MaxCubeTransferRate NUMBER_OF_BOXES / 4

#define SegmentBounarySize CubeSize * 2

#define MaxSpeed 2.5
#define MaxSpeedSquared MaxSpeed * MaxSpeed