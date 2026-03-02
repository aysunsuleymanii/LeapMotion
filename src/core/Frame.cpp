//
// Created by Aysun Suleymanturk on 2.3.26.
//

#include "Frame.h"

Frame::Frame() {}

Frame::Frame(const LEAP_TRACKING_EVENT* raw)
{
    if (!raw) return;

    for (uint32_t i = 0; i < raw->nHands; ++i)
    {
        hands_.emplace_back(raw->pHands[i]);
    }
}

const std::vector<Hand>& Frame::hands() const
{
    return hands_;
}