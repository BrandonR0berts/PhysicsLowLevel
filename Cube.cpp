#include "Cube.h"
#include "Commons.h"

#include <GL/glut.h>
#include <GL/freeglut.h>

// -----------------------------------------------------------------------------

void Box::UpdatePhysics(const float deltaTime)
{
    const float floorY  = 0.0f;
    const float gravity = -19.81f;

    // Update velocity due to gravity
    velocity.y += gravity * deltaTime;

    // Update position based on velocity
    position.x += velocity.x * deltaTime;
    position.y += velocity.y * deltaTime;
    position.z += velocity.z * deltaTime;

    // Check for collision with the floor
    if (position.y - halfSize.y < floorY)
    {
        position.y = floorY + halfSize.y;

        float dampening = 0.7f;
        velocity.y = -velocity.y * dampening;
    }

    // Check for collision with the walls
    if (position.x - halfSize.x < minX || position.x + halfSize.x > maxX)
    {
        velocity.x = -velocity.x;
    }
    if (position.z - halfSize.z < minZ || position.z + halfSize.z > maxZ)
    {
        velocity.z = -velocity.z;
    }

    CapVelocity();
}

// -----------------------------------------------------------------------------

void Box::Render()
{   
    glPushMatrix();
    glTranslatef(position.x, position.y, position.z);
    GLfloat diffuseMaterial[] = { colour.x, colour.y, colour.z, 1.0f };
    glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuseMaterial);
    glScalef(halfSize.x * 2.0f, halfSize.y * 2.0f, halfSize.z * 2.0f);
    glRotatef(-90, 1, 0, 0);
    glutSolidCube(1.0);
    glPopMatrix();
}

// -----------------------------------------------------------------------------

bool Box::CheckCollision(Box& other)
{
    return (std::abs(position.x - other.position.x) < (halfSize.x + other.halfSize.x)) &&
           (std::abs(position.y - other.position.y) < (halfSize.y + other.halfSize.y)) &&
           (std::abs(position.z - other.position.z) < (halfSize.z + other.halfSize.z));
}

// -----------------------------------------------------------------------------

void Box::ResolveCollision(Box& a, Box& b)
{
    Vec3 normal = { a.position.x - b.position.x, a.position.y - b.position.y, a.position.z - b.position.z };
    float length = std::sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);

    // Normalize the normal vector
    normal.normalise();

    float relativeVelocityX = a.velocity.x - b.velocity.x;
    float relativeVelocityY = a.velocity.y - b.velocity.y;
    float relativeVelocityZ = a.velocity.z - b.velocity.z;

    // Compute the relative velocity along the normal
    float impulse = relativeVelocityX * normal.x + relativeVelocityY * normal.y + relativeVelocityZ * normal.z;

    // Ignore collision if objects are moving away from each other
    if (impulse > 0) 
        return;

    // Compute the collision impulse scalar
    float e = 0.01f; // Coefficient of restitution (0 = inelastic, 1 = elastic)
    float dampening = 0.9f; // Dampening factor (0.9 = 10% energy reduction)
    float j = -(1.0f + e) * impulse * dampening;

    // Apply the impulse to the boxes' velocities
    a.velocity.x += j * normal.x;
    a.velocity.y += j * normal.y;
    a.velocity.z += j * normal.z;
    b.velocity.x -= j * normal.x;
    b.velocity.y -= j * normal.y;
    b.velocity.z -= j * normal.z;

    a.CapVelocity();
    b.CapVelocity();
}

// -----------------------------------------------------------------------------

void Box::CapVelocity()
{
    if (Vec3(velocity.x, 0.0f, velocity.z).lengthSquared() > MaxSpeedSquared)
    {
        velocity = velocity.normalised() * MaxSpeed;
    }
}

// -----------------------------------------------------------------------------