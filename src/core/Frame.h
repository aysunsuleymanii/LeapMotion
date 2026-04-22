#pragma once

#include <vector>
#include "Hand.h"
#include "../../external/ultraleap/include/LeapC.h"

class Frame {
public:
    Frame();
    explicit Frame(const LEAP_TRACKING_EVENT* raw);

    const std::vector<Hand>& hands() const;

private:
    std::vector<Hand> hands_;
};