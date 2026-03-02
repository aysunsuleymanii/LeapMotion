//
// Created by Aysun Suleymanturk on 1.3.26.
//

#ifndef LEAPMOTION_LEAPCONNECTION_H
#define LEAPMOTION_LEAPCONNECTION_H

#endif //LEAPMOTION_LEAPCONNECTION_H

#pragma once
// #include "LeapC.h"
#include "../../external/ultraleap/include/LeapC.h"

class LeapConnection {
public:
    LeapConnection();
    ~LeapConnection();

    bool open();
    void close();
    LEAP_CONNECTION getHandle() const;

private:
    LEAP_CONNECTION connection_;
};