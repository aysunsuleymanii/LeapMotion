//
// Created by Aysun Suleymanturk on 2.3.26.
//

#pragma once

#include "LeapConnection.h"
#include "../core/Frame.h"

/**
 * @file LeapAdapter.h
 * @brief Pulls tracking events off a LeapConnection and exposes them as Frames.
 */

/**
 * @class LeapAdapter
 * @brief Converts the SDK's polled message stream into engine Frame objects.
 *
 * Each poll() drains the SDK message queue and returns only the newest tracking
 * frame, which keeps end-to-end latency at roughly one frame even if processing
 * briefly falls behind the ~110 fps device.
 */
class LeapAdapter {
public:
    /** @brief Bind the adapter to an already-open connection. */
    explicit LeapAdapter(LeapConnection& connection);

    /**
     * @brief Poll for the latest tracking frame.
     * @param[out] outFrame Receives the most recent frame if one was available.
     * @return true if @p outFrame was populated this call, false if idle.
     */
    bool poll(Frame& outFrame);

private:
    LeapConnection& connection_;   ///< Non-owning reference to the connection.
};