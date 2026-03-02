#pragma once

struct HandState {
    float prevPinch = 0.0f;
    float prevGrab = 0.0f;
    bool dragging = false;
};