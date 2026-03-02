//
// Created by Aysun Suleymanturk on 1.3.26.
//

#include "HandTracker.h"
#include <iostream>

HandTracker::HandTracker(LEAP_CONNECTION connection)
        : connection_(connection) {}

void HandTracker::pollFrame() {
    LEAP_CONNECTION_MESSAGE msg;

    if (LeapPollConnection(connection_, 1000, &msg) == eLeapRS_Success) {

        if (msg.type == eLeapEventType_Tracking) {

            const LEAP_TRACKING_EVENT* frame = msg.tracking_event;

            std::cout << "Hands detected: "
                      << frame->nHands
                      << std::endl;

            for (uint32_t i = 0; i < frame->nHands; i++) {

                LEAP_HAND& hand = frame->pHands[i];

                std::cout << (hand.type == eLeapHandType_Left ? "Left" : "Right")
                          << " hand\n";

                std::cout << "  Palm position: "
                          << hand.palm.position.x << ", "
                          << hand.palm.position.y << ", "
                          << hand.palm.position.z << "\n";

                std::cout << "  Pinch: " << hand.pinch_strength << "\n";
                std::cout << "  Grab:  " << hand.grab_strength << "\n";
            }
        }
    }
}