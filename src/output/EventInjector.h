#pragma once

#include <CoreGraphics/CoreGraphics.h>

/**
 * @file EventInjector.h
 * @brief Turns detected gestures into real macOS input events.
 */

/**
 * @class EventInjector
 * @brief Static helpers that post synthetic input to the OS via CoreGraphics.
 *
 * This is the project's only point of contact with the operating system. Every
 * method builds one or more @c CGEvent objects (mouse moves/clicks, scroll
 * wheel, keyboard shortcuts for zoom/rotate, multi-touch swipe/smart-zoom) and
 * posts them to the HID event tap, so the rest of the app stays platform-free.
 *
 * Posting synthetic events requires the host application to be granted
 * Accessibility permission; checkAccessibility() verifies this at start-up.
 */
class EventInjector {
public:
    /**
     * @brief Verify the app has macOS Accessibility permission.
     * @return true if permitted to post events; false otherwise.
     */
    static bool checkAccessibility();

    /** @brief Move the system cursor to absolute screen point @p pos. */
    static void moveCursor(CGPoint pos);

    /** @brief Synthesize a left mouse click at @p pos. */
    static void leftClick(CGPoint pos);
    /** @brief Synthesize a right mouse click (context menu) at @p pos. */
    static void rightClick(CGPoint pos);

    /**
     * @brief Post a scroll-wheel event.
     * @param deltaY Vertical scroll amount (line units).
     * @param deltaX Horizontal scroll amount (default 0).
     */
    static void scroll(int32_t deltaY, int32_t deltaX = 0);

    /**
     * @brief Emit one zoom step via keyboard shortcut (Cmd + / Cmd -).
     * @param magnification Sign selects direction: >0 zoom in, <0 zoom out.
     * @param phase Reserved (gesture phase); currently unused.
     */
    static void zoom(float magnification, int phase);

    /**
     * @brief Emit one rotate step via keyboard shortcut (Cmd R / Cmd L).
     * @param degrees Sign selects direction: >0 rotate right, <0 rotate left.
     * @param phase Reserved (gesture phase); currently unused.
     */
    static void rotate(float degrees, int phase);

    /** @brief Post a left multi-touch swipe (back navigation). */
    static void swipeLeft();
    /** @brief Post a right multi-touch swipe (forward navigation). */
    static void swipeRight();

    /** @brief Press and hold the left mouse button at @p pos (begin drag). */
    static void mouseDown(CGPoint pos);
    /** @brief Move with the left button held at @p pos (continue drag). */
    static void mouseDragged(CGPoint pos);
    /** @brief Release the left mouse button at @p pos (end drag / drop). */
    static void mouseUp(CGPoint pos);
    /** @brief Trigger a smart-zoom (two-finger double tap) at @p pos. */
    static void smartZoom(CGPoint pos);
};