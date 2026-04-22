#include <iostream>

#include "input/LeapConnection.h"
#include "input/LeapAdapter.h"
#include "gesture/GestureDetector.h"
#include "core/Frame.h"
#include "output/EventInjector.h"

int main()
{
    // Check Accessibility permission before anything else
    if (!EventInjector::checkAccessibility())
        return -1;

    LeapConnection connection;
    if (!connection.open())
    {
        std::cerr << "Failed to open Leap connection\n";
        return -1;
    }

    std::cout << "[LeapMotion] Connected. Gesture controller running.\n";

    LeapAdapter    adapter(connection);
    GestureDetector gestures;

    while (true)
    {
        Frame frame;
        if (adapter.poll(frame))
        {
            gestures.update(frame);
        }
        // LeapPollConnection blocks up to 100 ms internally — no busy spin
    }

    return 0;
}