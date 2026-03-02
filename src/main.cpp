#include <iostream>

#include "input/LeapConnection.h"
#include "input/LeapAdapter.h"
#include "gesture/GestureDetector.h"
#include "core/Frame.h"

int main()
{
    LeapConnection connection;
    if (!connection.open())
    {
        std::cout << "Failed to open connection\n";
        return -1;
    }

    LeapAdapter adapter(connection);
    GestureDetector gestures;

    bool running = true;

    while (running)
    {
        Frame frame;

        if (adapter.poll(frame))
        {
            gestures.update(frame);
        }
    }

    return 0;
}