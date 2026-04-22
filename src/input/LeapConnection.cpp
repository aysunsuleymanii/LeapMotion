#include "LeapConnection.h"
#include <iostream>

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