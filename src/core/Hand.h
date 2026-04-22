#pragma once

#include "../math/Vector3.h"
#include "../../external/ultraleap/include/LeapC.h"

class Hand {
public:
    explicit Hand(const LEAP_HAND& raw);

    uint32_t id()           const noexcept;
    bool     isLeft()       const noexcept;
    float    pinch()        const noexcept;
    float    grab()         const noexcept;

    const Vector3& palmPosition() const noexcept;
    const Vector3& palmVelocity() const noexcept;

    const Vector3& fingerTip(int index) const noexcept;

    int extendedFingerCount() const noexcept;

private:
    uint32_t id_;
    bool     left_;
    float    pinch_;
    float    grab_;

    Vector3  palm_;
    Vector3  palmVel_;

    Vector3  tips_[5];
    bool     extended_[5];

    // 🔥 cached (important)
    int extendedCount_;
};