#include "LeapAdapter.h"
#include "../../external/ultraleap/include/LeapC.h"

LeapAdapter::LeapAdapter(LeapConnection& connection)
    : connection_(connection)
{
}

bool LeapAdapter::poll(Frame& outFrame)
{
    LEAP_CONNECTION_MESSAGE msg;

    if (LEAP_SUCCEEDED(
            LeapPollConnection(connection_.getHandle(), 100, &msg)))
    {
        if (msg.type == eLeapEventType_Tracking)
        {
            outFrame = Frame(msg.tracking_event);
            return true;
        }
    }

    return false;
}