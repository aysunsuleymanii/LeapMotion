//
// Created by Aysun Suleymanturk on 1.3.26.
//

#ifndef LEAPMOTION_LEAPCONNECTION_H
#define LEAPMOTION_LEAPCONNECTION_H

#endif //LEAPMOTION_LEAPCONNECTION_H

#pragma once
#include "../../external/ultraleap/include/LeapC.h"

/**
 * @file LeapConnection.h
 * @brief RAII wrapper around an Ultraleap LeapC connection handle.
 */

/**
 * @class LeapConnection
 * @brief Owns the lifetime of a connection to the Ultraleap tracking service.
 *
 * On open() it creates and opens a LeapC connection and requests Desktop
 * tracking mode (device sitting on the desk, looking up). The destructor closes
 * and destroys the handle, so callers never manage SDK resources directly.
 */
class LeapConnection {
public:
    /** @brief Construct with a null handle (call open() to connect). */
    LeapConnection();
    /** @brief Closes and destroys the connection if still open. */
    ~LeapConnection();

    /**
     * @brief Create + open the connection and select Desktop tracking mode.
     * @return true on success, false if the SDK could not create/open it.
     */
    bool open();
    /** @brief Close and destroy the connection (safe to call repeatedly). */
    void close();
    /** @brief Raw LeapC handle, for polling via LeapAdapter. */
    LEAP_CONNECTION getHandle() const;

private:
    LEAP_CONNECTION connection_;   ///< Underlying SDK connection handle.
};