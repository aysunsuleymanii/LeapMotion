#pragma once

#include "../math/Vector3.h"
#include "../../external/ultraleap/include/LeapC.h"

class Hand {
public:
    Hand(const LEAP_HAND& raw);

    bool isLeft() const;
    float pinch() const;
    float grab() const;
    Vector3 palmPosition() const;

private:
    bool left_;
    float pinch_;
    float grab_;
    Vector3 palm_;
};