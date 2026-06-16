#pragma once
#include <cmath>

/**
 * @file Vector3.h
 * @brief Minimal 3D vector used throughout the project for positions,
 *        velocities and direction maths.
 */

/**
 * @struct Vector3
 * @brief A lightweight 3-component (x, y, z) float vector.
 *
 * Ultraleap reports all spatial data (palm position, fingertip joints, palm
 * velocity) in millimetres in the device coordinate frame. Vector3 wraps those
 * triples and provides the small set of operations the gesture layer needs:
 * subtraction (to form direction vectors between joints), length/distance, and
 * the planar angle helpers used by the rotate gesture.
 */
struct Vector3 {
    float x = 0, y = 0, z = 0;   ///< Components in millimetres (device frame).

    Vector3() = default;
    /** @brief Construct from explicit components. */
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    /** @brief Component-wise addition. */
    Vector3 operator+(const Vector3& o) const { return {x+o.x, y+o.y, z+o.z}; }
    /** @brief Component-wise subtraction (used to build joint-to-joint vectors). */
    Vector3 operator-(const Vector3& o) const { return {x-o.x, y-o.y, z-o.z}; }
    /** @brief Scalar multiplication. */
    Vector3 operator*(float s)          const { return {x*s,   y*s,   z*s};   }

    /** @brief Dot product. */
    float dot(const Vector3& o)   const { return x*o.x + y*o.y + z*o.z; }
    /** @brief Euclidean length (magnitude) of the vector. */
    float length()                const { return std::sqrt(x*x + y*y + z*z); }
    /** @brief Euclidean distance between this point and @p o. */
    float distanceTo(const Vector3& o) const { return (*this - o).length(); }

    /** @brief Unit vector in the same direction (zero vector if length is 0). */
    Vector3 normalized() const {
        float l = length();
        return l > 0 ? Vector3{x/l, y/l, z/l} : Vector3{};
    }

    /** @brief Angle (radians) of the X-Y projection — rotation gesture in HMD mode. */
    float xyAngle() const { return std::atan2(y, x); }
    /** @brief Angle (radians) of the X-Z projection — rotation gesture in Desktop mode. */
    float xzAngle() const { return std::atan2(z, x); }
};