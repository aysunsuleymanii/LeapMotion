#include "LeapAdapter.h"
#include "../../external/ultraleap/include/LeapC.h"

LeapAdapter::LeapAdapter(LeapConnection& connection)
    : connection_(connection)
{
}

bool LeapAdapter::poll(Frame& outFrame)
{
    LEAP_CONNECTION_MESSAGE msg;
    bool got = false;

    // Block briefly for at least one message so we don't busy-spin when idle.
    if (LEAP_SUCCEEDED(
            LeapPollConnection(connection_.getHandle(), 100, &msg)))
    {
        if (msg.type == eLeapEventType_Tracking)
        {
            outFrame = Frame(msg.tracking_event);
            got = true;
        }
    }

    // Drain everything else already queued, without waiting (timeout 0), and
    // keep only the MOST RECENT tracking frame. The Leap service produces
    // frames at ~110 fps; if our processing ever falls behind, the queue backs
    // up and we'd be acting on older and older frames — that is the lag. By
    // discarding the backlog each loop, latency stays at ~one frame.
    while (LEAP_SUCCEEDED(
            LeapPollConnection(connection_.getHandle(), 0, &msg)))
    {
        if (msg.type == eLeapEventType_Tracking)
        {
            outFrame = Frame(msg.tracking_event);
            got = true;
        }
    }

    return got;
}