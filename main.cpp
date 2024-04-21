#include <stdlib.h>
#include <GL/glut.h>
#include <GL/freeglut.h>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <chrono>

#include "TimeTracker.h"
#include "Cube.h"
#include "Vector3D.h"
#include "Quadtree.h"

#include "Commons.h"

#if UseMemoryTracking
    #include "GlobalTrackers.h"
#endif

#if UseMemoryPools
    #include "MemoryPool.h"
#endif

// Trackers - overall
TimeTracker sIdleTimeTracker;
TimeTracker sRenderTimeTracker;

// Trackers - specific functions
#if FINE_TUNED_MEASUREMENTS == true
    TimeTracker mPhysicsTimeTracker;
    TimeTracker mDrawSceneTimeTracker;
#endif

// --------------------------------------------------------------------------------------------------- //

    Quadtree* sQuadtree = nullptr;

// --------------------------------------------------------------------------------------------------- //

void initScene(int boxCount) 
{
    float xRange = maxX - minX;
    float zRange = maxX - minX;

    for (int i = 0; i < boxCount; ++i) 
    {
        Box box;

        // Assign random x, y, and z positions within specified ranges
        box.position.x = minX + (static_cast<float>(rand()) / (static_cast<float>(RAND_MAX))) * xRange;
        box.position.y = 10.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 1.0f));
        box.position.z = minZ + (static_cast<float>(rand()) / (static_cast<float>(RAND_MAX))) * zRange;

        float halfSize = CubeSize / 2.0f;
        box.halfSize   = { halfSize, halfSize, halfSize };

        // Assign random x-velocity between -1.0f and 1.0f
        float randomXVelocity = -1.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 2.0f));
        box.velocity = {randomXVelocity, 0.0f, 0.0f};

        // Assign a random color to the box
#if !VisualiseQuadtree
        box.colour.x = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        box.colour.y = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        box.colour.z = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
#else
        box.colour = Vec3(1.0f, 0.0f, 0.0f);
#endif
        sQuadtree->AddCubeToTree(box);
    }
}

// --------------------------------------------------------------------------------------------------- //

bool rayBoxIntersection(const Vec3& rayOrigin, const Vec3& rayDirection, const Box& box)
{
    float tMin = (box.position.x - box.halfSize.x - rayOrigin.x) / rayDirection.x;
    float tMax = (box.position.x + box.halfSize.x - rayOrigin.x) / rayDirection.x;

    if (tMin > tMax) std::swap(tMin, tMax);

    float tyMin = (box.position.y - box.halfSize.y - rayOrigin.y) / rayDirection.y;
    float tyMax = (box.position.y + box.halfSize.y - rayOrigin.y) / rayDirection.y;

    if (tyMin > tyMax) std::swap(tyMin, tyMax);

    if ((tMin > tyMax) || (tyMin > tMax))
        return false;

    if (tyMin > tMin)
        tMin = tyMin;

    if (tyMax < tMax)
        tMax = tyMax;

    float tzMin = (box.position.z - box.halfSize.z - rayOrigin.z) / rayDirection.z;
    float tzMax = (box.position.z + box.halfSize.z - rayOrigin.z) / rayDirection.z;

    if (tzMin > tzMax) std::swap(tzMin, tzMax);

    if ((tMin > tzMax) || (tzMin > tMax))
        return false;

    return true;
}

// --------------------------------------------------------------------------------------------------- //

Vec3 screenToWorld(int x, int y) 
{
    GLint    viewport[4];
    GLdouble modelview[16];
    GLdouble projection[16];
    GLfloat  winX, winY, winZ;
    GLdouble posX, posY, posZ;

    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);

    winX = (float)x;
    winY = (float)viewport[3] - (float)y;
    glReadPixels(x, int(winY), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);

    gluUnProject(winX, winY, winZ, modelview, projection, viewport, &posX, &posY, &posZ);

    return Vec3((float)posX, (float)posY, (float)posZ);
}

// --------------------------------------------------------------------------------------------------- //

void updatePhysics(const float deltaTime) 
{
#if FINE_TUNED_MEASUREMENTS == true
    mPhysicsTimeTracker.StartTiming();
#endif
    {      
        // Update the physics of the cubes
        sQuadtree->Update(deltaTime);

        // Now check for collisions
        sQuadtree->CheckCollisions();
    }

#if FINE_TUNED_MEASUREMENTS == true
    mPhysicsTimeTracker.AddMeasurement();
#endif
}

// --------------------------------------------------------------------------------------------------- //

// draw the sides of the containing area
void drawQuad(const Vec3& v1, const Vec3& v2, const Vec3& v3, const Vec3& v4) 
{
    glBegin(GL_QUADS);
    glVertex3f(v1.x, v1.y, v1.z);
    glVertex3f(v2.x, v2.y, v2.z);
    glVertex3f(v3.x, v3.y, v3.z);
    glVertex3f(v4.x, v4.y, v4.z);
    glEnd();
}

// --------------------------------------------------------------------------------------------------- //

void drawScene() 
{
#if FINE_TUNED_MEASUREMENTS == true
    mDrawSceneTimeTracker.StartTiming();
#endif
    {    // Draw the side wall
        GLfloat diffuseMaterial[] = { 0.2f, 0.2f, 0.2f, 1.0f };
        glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuseMaterial);

        // Draw the left side wall
        glColor3f(0.5f, 0.5f, 0.5f); // Set the wall color
        Vec3 leftSideWallV1(minX, 0.0f, maxZ);
        Vec3 leftSideWallV2(minX, 50.0f, maxZ);
        Vec3 leftSideWallV3(minX, 50.0f, minZ);
        Vec3 leftSideWallV4(minX, 0.0f, minZ);
        drawQuad(leftSideWallV1, leftSideWallV2, leftSideWallV3, leftSideWallV4);

        // Draw the right side wall
        glColor3f(0.5f, 0.5f, 0.5f); // Set the wall color
        Vec3 rightSideWallV1(maxX, 0.0f, maxZ);
        Vec3 rightSideWallV2(maxX, 50.0f, maxZ);
        Vec3 rightSideWallV3(maxX, 50.0f, minZ);
        Vec3 rightSideWallV4(maxX, 0.0f, minZ);
        drawQuad(rightSideWallV1, rightSideWallV2, rightSideWallV3, rightSideWallV4);


        // Draw the back wall
        glColor3f(0.5f, 0.5f, 0.5f); // Set the wall color
        Vec3 backWallV1(minX, 0.0f, minZ);
        Vec3 backWallV2(minX, 50.0f, minZ);
        Vec3 backWallV3(maxX, 50.0f, minZ);
        Vec3 backWallV4(maxX, 0.0f, minZ);
        drawQuad(backWallV1, backWallV2, backWallV3, backWallV4);

        // Now use the quadtree to draw all of the boxes
        sQuadtree->Render();
    }

#if FINE_TUNED_MEASUREMENTS == true
        mDrawSceneTimeTracker.AddMeasurement();
#endif
}

// --------------------------------------------------------------------------------------------------- //

void display() 
{
    sRenderTimeTracker.StartTiming();
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();

        gluLookAt(LOOKAT_X, LOOKAT_Y, LOOKAT_Z, LOOKDIR_X, LOOKDIR_Y, LOOKDIR_Z, 0, 1, 0);

        drawScene();

        glutSwapBuffers();
    }
    sRenderTimeTracker.AddMeasurement();
}

// --------------------------------------------------------------------------------------------------- //

void idle() 
{
    static std::chrono::steady_clock::time_point last      = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point        old       = last;
		                                         last      = std::chrono::steady_clock::now();

	const std::chrono::duration<float>           frameTime = last - old;
	float                                        deltaTime = frameTime.count();

    // If there are 0 threads allocated then we need to do it all on the main thread like before
    if (ThreadsToAllocateToProgram > 0)
    {
        sQuadtree->Update(deltaTime);

        glutPostRedisplay();

        return;
    }

    sIdleTimeTracker.StartTiming();
     
        updatePhysics(deltaTime);

        // tell glut to draw - note this will cap this function at 60 fps
        glutPostRedisplay();
    
    sIdleTimeTracker.AddMeasurement();
}

// --------------------------------------------------------------------------------------------------- //

// called the mouse button is tapped
void mouse(int button, int state, int x, int y) 
{
    //if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) 
    //{
    //    // Get the camera position and direction
    //    Vec3 cameraPosition(LOOKAT_X, LOOKAT_Y, LOOKAT_Z); // Replace with your actual camera position
    //    Vec3 cameraDirection(LOOKDIR_X, LOOKDIR_Y, LOOKDIR_Z); // Replace with your actual camera direction

    //    // Get the world coordinates of the clicked point
    //    Vec3 clickedWorldPos = screenToWorld(x, y);

    //    // Calculate the ray direction from the camera position to the clicked point
    //    Vec3 rayDirection = clickedWorldPos - cameraPosition;
    //    rayDirection.normalise();

    //    // Perform a ray-box intersection test and remove the clicked box
    //    size_t clickedBoxIndex = -1;
    //    float minIntersectionDistance = std::numeric_limits<float>::max();

    //    for (size_t i = 0; i < boxes.size(); ++i)
    //    {
    //        if (rayBoxIntersection(cameraPosition, rayDirection, boxes[i])) 
    //        {
    //            // Calculate the distance between the camera and the intersected box
    //            Vec3 diff = boxes[i].position - cameraPosition;
    //            float distance = diff.length();

    //            // Update the clicked box index if this box is closer to the camera
    //            if (distance < minIntersectionDistance) 
    //            {
    //                clickedBoxIndex = i;
    //                minIntersectionDistance = distance;
    //            }
    //        }
    //    }

    //    // Remove the clicked box if any
    //    if (clickedBoxIndex != -1) 
    //    {
    //        boxes.erase(boxes.begin() + clickedBoxIndex);
    //    }
    //}
}

// --------------------------------------------------------------------------------------------------- //

// called when the keyboard is used
void keyboard(unsigned char key, int x, int y) 
{
    const float impulseMagnitude = 20.0f; // Upward impulse magnitude

    if (key == ' ') 
    { 
        sQuadtree->ImpulseAllBoxes(impulseMagnitude);
    }
#if UseMemoryTracking
    else if (key == '1')
    {
        if(Memory::mGenericTracker)
            Memory::mGenericTracker->WalkTheHeap();
    }
#endif

#if UseMemoryPools
    else if (key == '2')
    {
        Memory::MemoryPool::Get()->GetMutex()->lock();

            Memory::MemoryPool::Get()->DebugOutputUsage(true, false);

        Memory::MemoryPool::Get()->GetMutex()->unlock();
    }
    else if (key == '3')
    {
        Memory::MemoryPool::Get()->GetMutex()->lock();

            Memory::MemoryPool::Get()->DebugOutputUsage(false, true);

        Memory::MemoryPool::Get()->GetMutex()->unlock();
    }
#endif
}

// --------------------------------------------------------------------------------------------------- //

void OutputFinalMeasurements()
{
    std::cout << "Update timings: ";
    sQuadtree->GetTimeTracker().OutputAverageTime();

    // Idle function time taken - will be 0 if everything else is run from seperate threads
    std::cout << "Idle function average time: ";
    sIdleTimeTracker.OutputAverageTime();

    std::cout << "Average time taken for the Display function: ";
    sRenderTimeTracker.OutputAverageTime();

#if FINE_TUNED_MEASUREMENTS == true
    std::cout << "Average time for update physics function call: ";
    mPhysicsTimeTracker.OutputAverageTime();

    std::cout << "Average time for draw scene function call: ";
    mDrawSceneTimeTracker.OutputAverageTime();
#endif
}

// --------------------------------------------------------------------------------------------------- //

#include <xmmintrin.h>

class testLarge
{
    float test;
};

int main(int argc, char** argv) 
{
#if UseMemoryPools
    // Getting here so that it is setup
    Memory::MemoryPool::Get()->Init();
    Memory::MemoryPool::Get()->InitMutex();
#endif

    std::cout << "Time taken for setup: ";
    sIdleTimeTracker.StartTiming();
    {
        // Setup
        srand(static_cast<unsigned>(time(0))); // Seed random number generator
        glutInit(&argc, argv);
        glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
        glutInitWindowSize(1920, 1080);
        glutCreateWindow("Simple Physics Simulation");

        // Make freeglut return back into this function so that we can output the data calculated
        glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

        // Provide the callbacks
        glutKeyboardFunc(keyboard);
        glutMouseFunc(mouse);

        // opengl setup
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(45.0, 800.0 / 600.0, 0.1, 100.0);
        glMatrixMode(GL_MODELVIEW);

#if UseMemoryTracking
        Memory::SetupGlobalTrackers();
#endif

        sQuadtree = new Quadtree(QuadtreeDepth, { minX, 0.0f, minZ }, { maxX, 1.0f, maxZ });

        initScene(NUMBER_OF_BOXES);

        // Provide the render callbacks
        glutDisplayFunc(display);
        glutIdleFunc(idle);

    }
    sIdleTimeTracker.AddMeasurement();
    sIdleTimeTracker.StopTiming();
    sIdleTimeTracker.OutputAverageTime();
    sIdleTimeTracker.Clear();

    glutMainLoop();

    OutputFinalMeasurements();

    delete sQuadtree;
    sQuadtree = nullptr;

    return 0;
}

// --------------------------------------------------------------------------------------------------- //