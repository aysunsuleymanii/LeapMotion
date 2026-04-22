#pragma once
#include <cmath>

struct Vector3 {
    float x = 0, y = 0, z = 0;

    Vector3() = default;
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vector3 operator+(const Vector3& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Vector3 operator-(const Vector3& o) const { return {x-o.x, y-o.y, z-o.z}; }
    Vector3 operator*(float s)          const { return {x*s,   y*s,   z*s};   }

    float dot(const Vector3& o)   const { return x*o.x + y*o.y + z*o.z; }
    float length()                const { return std::sqrt(x*x + y*y + z*z); }
    float distanceTo(const Vector3& o) const { return (*this - o).length(); }

    Vector3 normalized() const {
        float l = length();
        return l > 0 ? Vector3{x/l, y/l, z/l} : Vector3{};
    }

    // Angle (radians) of the XY projection — used for rotation gesture
    float xyAngle() const { return std::atan2(y, x); }
};