/**
* @file LeapConnection.cpp
 * @brief Opens/closes the Ultraleap connection and selects Desktop mode.
 */
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
    config.size = sizeof(config);

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

    for (int attempt = 0; attempt < 20; ++attempt) {
        if (LeapSetTrackingMode(connection_, eLeapTrackingMode_Desktop)
            == eLeapRS_Success)
        {
            std::cerr << "[LeapConnection] tracking mode set to Desktop\n";
            break;
        }
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