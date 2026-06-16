#pragma once

#include <vector>
#include "Hand.h"
#include "../../external/ultraleap/include/LeapC.h"

/**
 * @file Frame.h
 * @brief One frame of tracking data: the set of hands seen at an instant.
 */

/**
 * @class Frame
 * @brief A single tracking frame holding zero or more Hand objects.
 *
 * Frame wraps a raw @c LEAP_TRACKING_EVENT and converts each contained hand
 * into a Hand. The gesture detector consumes one Frame per processed update.
 */
class Frame {
public:
    /** @brief Construct an empty frame (no hands). */
    Frame();
    /**
     * @brief Construct from a raw SDK tracking event.
     * @param raw Pointer to the SDK event; null/empty events yield no hands.
     */
    explicit Frame(const LEAP_TRACKING_EVENT* raw);

    /** @brief The hands present in this frame. */
    const std::vector<Hand>& hands() const;

private:
    std::vector<Hand> hands_;   ///< Hands decoded from the tracking event.
};