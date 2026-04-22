#pragma once

#include "../math/Vector3.h"
#include "../../external/ultraleap/include/LeapC.h"

class Hand {
public:
    explicit Hand(const LEAP_HAND& raw);

    uint32_t id()           const noexcept;
    bool     isLeft()       const noexcept;
    float    pinch()        const noexcept;          // 0..1 strength
    float    pinchDistance() const noexcept;         // mm, thumb↔index
    float    grab()         const noexcept;          // 0..1 strength

    const Vector3& palmPosition() const noexcept;
    const Vector3& palmVelocity() const noexcept;

    // Finger tips: 0=thumb 1=index 2=middle 3=ring 4=pinky
    const Vector3& fingerTip(int index) const noexcept;
    bool           isExtended(int index) const noexcept;

    int extendedFingerCount() const noexcept;

private:
    uint32_t id_;
    bool     left_;
    float    pinch_;
    float    pinchDistance_;
    float    grab_;

    Vector3  palm_;
    Vector3  palmVel_;

    Vector3  tips_[5];
    bool     extended_[5];

    // 🔥 cached (important)
    int extendedCount_;
};