/**
* @file main.cpp
 * @brief Program entry point: wires the pipeline and runs the gesture loop.
 *
 * Flow: verify Accessibility permission → open the Leap connection → poll
 * frames through LeapAdapter → feed each frame to GestureDetector, which posts
 * the corresponding system events via EventInjector.
 */
#include <iostream>

#include "input/LeapConnection.h"
#include "input/LeapAdapter.h"
#include "gesture/GestureDetector.h"
#include "core/Frame.h"
#include "output/EventInjector.h"

/**
 * @brief Application entry point.
 * @return 0 normally; -1 if Accessibility permission or the Leap connection
 *         could not be established.
 */
int main() {
    // Check Accessibility permission before anything else
    if (!EventInjector::checkAccessibility())
        return -1;

    LeapConnection connection;
    if (!connection.open()) {
        std::cerr << "Failed to open Leap connection\n";
        return -1;
    }

    std::cout << "[LeapMotion] Connected. Gesture controller running.\n";

    LeapAdapter adapter(connection);
    GestureDetector gestures;

    // Main loop: one tracking frame in, the matching gesture out.
    while (true) {
        Frame frame;
        if (adapter.poll(frame)) {
            gestures.update(frame);
        }
    }
}