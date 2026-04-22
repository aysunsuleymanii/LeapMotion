#include "Hand.h"

Hand::Hand(const LEAP_HAND &raw) {
    id_ = raw.id;
    left_ = (raw.type == eLeapHandType_Left);
    pinch_         = raw.pinch_strength;
    pinchDistance_ = raw.pinch_distance;
    grab_          = raw.grab_strength;

    // ── Palm ──
    palm_ = {
        raw.palm.position.x,
        raw.palm.position.y,
        raw.palm.position.z
    };

    palmVel_ = {
        raw.palm.velocity.x,
        raw.palm.velocity.y,
        raw.palm.velocity.z
    };

    // ── Fingers ──
    extendedCount_ = 0;

    for (int i = 0; i < 5; ++i) {
        const LEAP_DIGIT &digit = raw.digits[i];

        tips_[i] = {
            digit.bones[3].next_joint.x,
            digit.bones[3].next_joint.y,
            digit.bones[3].next_joint.z
        };

        extended_[i] = digit.is_extended;

        if (extended_[i]) ++extendedCount_;
    }
}

// ── Getters (all noexcept + zero-copy) ──

uint32_t Hand::id() const noexcept { return id_; }
bool Hand::isLeft() const noexcept { return left_; }
float Hand::pinch() const noexcept { return pinch_; }
float Hand::pinchDistance() const noexcept { return pinchDistance_; }
float Hand::grab() const noexcept { return grab_; }

const Vector3 &Hand::palmPosition() const noexcept { return palm_; }
const Vector3 &Hand::palmVelocity() const noexcept { return palmVel_; }

const Vector3 &Hand::fingerTip(int index) const noexcept {
    static Vector3 zero{};
    return (index >= 0 && index < 5) ? tips_[index] : zero;
}

bool Hand::isExtended(int index) const noexcept {
    return (index >= 0 && index < 5) ? extended_[index] : false;
}

int Hand::extendedFingerCount() const noexcept {
    return extendedCount_;
}