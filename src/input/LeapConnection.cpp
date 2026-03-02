//
// Created by Aysun Suleymanturk on 1.3.26.
//
#include "LeapConnection.h"
#include <iostream>

LeapConnection::LeapConnection() : connection_(nullptr) {}

LeapConnection::~LeapConnection() {
    close();
}

bool LeapConnection::open() {
    LEAP_CONNECTION_CONFIG config = {0};

    if (LeapCreateConnection(&config, &connection_) != eLeapRS_Success)
        return false;

    if (LeapOpenConnection(connection_) != eLeapRS_Success)
        return false;

    return true;
}

void LeapConnection::close() {
    if (connection_) {
        LeapCloseConnection(connection_);
        LeapDestroyConnection(connection_);
        connection_ = nullptr;
    }
}

LEAP_CONNECTION LeapConnection::getHandle() const {
    return connection_;
}