/**
* @file Frame.cpp
 * @brief Builds a Frame (vector of Hand) from a raw tracking event.
 */
#include "Frame.h"

Frame::Frame() = default;

/** @brief Decode every hand in the event into a Hand object. */
Frame::Frame(const LEAP_TRACKING_EVENT* raw)
{
    if (!raw || raw->nHands == 0 || !raw->pHands)
        return;

    hands_.reserve(raw->nHands);

    for (uint32_t i = 0; i < raw->nHands; ++i)
    {
        hands_.emplace_back(raw->pHands[i]);
    }
}

const std::vector<Hand>& Frame::hands() const
{
    return hands_;
}