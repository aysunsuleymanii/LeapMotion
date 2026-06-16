#pragma once

/**
 * @file GestureState.h
 * @brief Minimal early per-hand state struct (superseded by the richer
 *        HandState defined in GestureDetector.h; kept for reference).
 */

struct HandState {
    float prevPinch = 0.0f;
    float prevGrab = 0.0f;
    bool dragging = false;
};