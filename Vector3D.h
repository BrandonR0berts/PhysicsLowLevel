#pragma once

#include <cmath>

class Vec3
{
public:
    float x, y, z;

    Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vec3 operator-(const Vec3& other) const
    {
        return Vec3(x - other.x, y - other.y, z - other.z);
    }

    Vec3 operator+(const Vec3& other) const
    {
        return Vec3(x + other.x, y + other.y, z + other.z);
    }

    Vec3& operator+=(const Vec3& other)
    {
        x += other.x;
        y += other.y;
        z += other.z;

        return *this;
    }

    Vec3 operator*(const float& other) const
    {
        return Vec3(x * other, y * other, z * other);
    }

    Vec3 operator/(const float& other) const
    {
        if (other == 0.0f)
            return Vec3();

        return Vec3(x / other, y / other, z / other);
    }

    void normalise()
    {
        float length = std::sqrt(x * x + y * y + z * z);
        if (length != 0)
        {
            x /= length;
            y /= length;
            z /= length;
        }
    }

    Vec3 normalised()
    {
        float length = std::sqrt(x * x + y * y + z * z);
        if (length != 0)
        {
            return Vec3(x / length, y / length, z / length);
        }

        return *this;
    }

    float length() const
    {
        return std::sqrt(x * x + y * y + z * z);
    }

    float lengthSquared() const
    {
         return (x * x + y * y + z * z);
    }
};