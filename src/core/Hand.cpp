//
// Created by Aysun Suleymanturk on 2.3.26.
//

#include "Hand.h"

Hand::Hand(const LEAP_HAND& raw)
{
    left_ = (raw.type == eLeapHandType_Left);
    pinch_ = raw.pinch_strength;
    grab_ = raw.grab_strength;

    palm_ = Vector3(
        raw.palm.position.x,
        raw.palm.position.y,
        raw.palm.position.z
    );
}

bool Hand::isLeft() const { return left_; }
float Hand::pinch() const { return pinch_; }
float Hand::grab() const { return grab_; }
Vector3 Hand::palmPosition() const { return palm_; }