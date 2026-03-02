//
// Created by Aysun Suleymanturk on 2.3.26.
//
#pragma once
#include "Hand.h"


#pragma once

#include <vector>


class Frame {
public:
    Frame();
    Frame(const LEAP_TRACKING_EVENT* raw);

    const std::vector<Hand>& hands() const;

private:
    std::vector<Hand> hands_;
};