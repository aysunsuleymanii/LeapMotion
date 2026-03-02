//
// Created by Aysun Suleymanturk on 2.3.26.
//

#pragma once

#include "LeapConnection.h"
#include "../core/Frame.h"

class LeapAdapter {
public:
    explicit LeapAdapter(LeapConnection& connection);

    bool poll(Frame& outFrame);

private:
    LeapConnection& connection_;
};