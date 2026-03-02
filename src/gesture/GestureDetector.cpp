//
// Created by Aysun Suleymanturk on 2.3.26.
//

#include "GestureDetector.h"
#include <iostream>

void GestureDetector::update(const Frame& frame)
{
    for (const auto& hand : frame.hands())
    {
        detectPinch(hand);
    }
}

void GestureDetector::detectPinch(const Hand& hand)
{
    int id = reinterpret_cast<std::uintptr_t>(&hand); // simple temporary id

    auto& state = states_[id];

    if (hand.pinch() > 0.9f && state.prevPinch < 0.3f)
    {
        std::cout << "Click detected!\n";
    }

    state.prevPinch = hand.pinch();
}