#include "LeapConnection.h"
#include <iostream>
#include <ctime>

LeapConnection::LeapConnection() : connection_(nullptr) {}

LeapConnection::~LeapConnection()
{
    close();
}

bool LeapConnection::open()
{
    LEAP_CONNECTION_CONFIG config = {};
    config.size = sizeof(config);   // ← required by LeapC; was missing before

    if (LeapCreateConnection(&config, &connection_) != eLeapRS_Success)
    {
        std::cerr << "[LeapConnection] LeapCreateConnection failed\n";
        return false;
    }

    if (LeapOpenConnection(connection_) != eLeapRS_Success)
    {
        std::cerr << "[LeapConnection] LeapOpenConnection failed\n";
        return false;
    }

    // Force desktop orientation (device flat on desk, LEDs facing up, hand
    // coming in from above). Without this the SDK may sit in HMD or
    // ScreenTop mode from a previous app, which reports finger extension
    // flags as if the hand is in a completely different orientation.
    // We retry a few times because LeapSetTrackingMode is an async request
    // that only succeeds once the device policy layer is ready.
    for (int attempt = 0; attempt < 20; ++attempt) {
        if (LeapSetTrackingMode(connection_, eLeapTrackingMode_Desktop)
            == eLeapRS_Success)
        {
            std::cerr << "[LeapConnection] tracking mode set to Desktop\n";
            break;
        }
        // Brief wait for the connection to finish initializing
        struct timespec ts{0, 50'000'000};   // 50 ms
        nanosleep(&ts, nullptr);
    }

    return true;
}

void LeapConnection::close()
{
    if (connection_) {
        LeapCloseConnection(connection_);
        LeapDestroyConnection(connection_);
        connection_ = nullptr;
    }
}

LEAP_CONNECTION LeapConnection::getHandle() const
{
    return connection_;
}