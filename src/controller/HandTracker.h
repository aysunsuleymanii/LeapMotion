//
// Created by Aysun Suleymanturk on 1.3.26.
//

#ifndef LEAPMOTION_HANDTRACKER_H
#define LEAPMOTION_HANDTRACKER_H

#endif //LEAPMOTION_HANDTRACKER_H

#pragma once
#include "LeapC.h"

class HandTracker {
public:
    explicit HandTracker(LEAP_CONNECTION connection);

    void pollFrame();

private:
    LEAP_CONNECTION connection_;
};