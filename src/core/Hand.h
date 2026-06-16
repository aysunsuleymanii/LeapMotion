#pragma once

#include "../math/Vector3.h"
#include "../../external/ultraleap/include/LeapC.h"

/**
 * @file Hand.h
 * @brief Clean, engine-friendly view of a single tracked hand.
 */

/**
 * @class Hand
 * @brief Immutable snapshot of one hand for a single frame.
 *
 * The Ultraleap SDK reports a hand as a raw @c LEAP_HAND C struct full of
 * nested joints and bones. Hand copies out only the quantities the gesture
 * layer actually uses — pinch/grab strengths, palm position and velocity, the
 * five fingertip positions and their extended flags — into simple typed
 * accessors. This keeps the rest of the codebase free of LeapC details.
 *
 * A Hand is constructed once per frame from the SDK data and is read-only
 * thereafter.
 */
class Hand {
public:
    /**
     * @brief Build a Hand from the SDK's raw hand struct.
     * @param raw One hand entry from a LEAP_TRACKING_EVENT.
     */
    explicit Hand(const LEAP_HAND& raw);

    /** @brief Stable per-hand id assigned by the tracking service. */
    uint32_t id()           const noexcept;
    /** @brief True if this is the left hand. */
    bool     isLeft()       const noexcept;
    /** @brief Pinch strength, 0 (open) … 1 (thumb and index touching). */
    float    pinch()        const noexcept;
    /** @brief Distance in mm between thumb and index tips. */
    float    pinchDistance() const noexcept;
    /** @brief Grab/fist strength, 0 (open) … 1 (closed fist). */
    float    grab()         const noexcept;

    /** @brief Palm centre position (mm, device frame). */
    const Vector3& palmPosition() const noexcept;
    /** @brief Palm velocity (mm/s, device frame). */
    const Vector3& palmVelocity() const noexcept;

    /**
     * @brief Fingertip position of one finger.
     * @param index 0=thumb, 1=index, 2=middle, 3=ring, 4=pinky.
     * @return Tip position (mm); a zero vector for an out-of-range index.
     */
    const Vector3& fingerTip(int index) const noexcept;
    /**
     * @brief Whether a finger is currently extended (straightened).
     * @param index 0=thumb … 4=pinky.
     */
    bool           isExtended(int index) const noexcept;

    /** @brief Number of fingers (0–5) currently extended. */
    int extendedFingerCount() const noexcept;

private:
    uint32_t id_;             ///< Tracking id.
    bool     left_;           ///< Left-hand flag.
    float    pinch_;          ///< Pinch strength 0..1.
    float    pinchDistance_;  ///< Thumb–index distance (mm).
    float    grab_;           ///< Grab strength 0..1.

    Vector3  palm_;           ///< Palm position (mm).
    Vector3  palmVel_;        ///< Palm velocity (mm/s).

    Vector3  tips_[5];        ///< Fingertip positions, thumb→pinky.
    bool     extended_[5];    ///< Per-finger extended flags.

    int extendedCount_;       ///< Cached count of extended fingers.
};