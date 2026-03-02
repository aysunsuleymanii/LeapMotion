#pragma once

#include <unordered_map>
#include "../core/Frame.h"
#include "GestureState.h"

class GestureDetector {
public:
    void update(const Frame& frame);

private:
    std::unordered_map<int, HandState> states_;

    void detectPinch(const Hand& hand);
};