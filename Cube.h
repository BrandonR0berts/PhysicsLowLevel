#pragma once

#include "Vector3D.h"

// the box (falling item)
struct Box
{
public:
    void UpdatePhysics(const float deltaTime);
    void Render();

    bool CheckCollision(Box& other);

    static void ResolveCollision(Box& a, Box& b);
    void CapVelocity();

    Vec3 position;
    Vec3 halfSize;
    Vec3 velocity;
    Vec3 colour;
};