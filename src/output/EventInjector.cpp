/**
 * @file EventInjector.cpp
 * @brief CoreGraphics implementations that post synthetic input to macOS.
 */
#include "EventInjector.h"
#include <ApplicationServices/ApplicationServices.h>
#include <iostream>
#include <ctime>

static CGPoint currentCursorPos() {
    CGEventRef e = CGEventCreate(NULL);
    CGPoint p = CGEventGetLocation(e);
    CFRelease(e);
    return p;
}


/** @brief Verify (and prompt for) macOS Accessibility permission. */
bool EventInjector::checkAccessibility() {
    if (!AXIsProcessTrusted()) {
        std::cerr << "[LeapMotion] Accessibility permission required.\n"
                << "  Go to: System Settings → Privacy & Security → Accessibility\n"
                << "  Add this app, then re-launch.\n";
        return false;
    }
    return true;
}

/** @brief Warp the system cursor to an absolute screen point. */
void EventInjector::moveCursor(CGPoint pos) {
    CGWarpMouseCursorPosition(pos);
    CGEventRef e = CGEventCreateMouseEvent(NULL, kCGEventMouseMoved, pos, kCGMouseButtonLeft);
    CGEventPost(kCGHIDEventTap, e);
    CFRelease(e);
}

/** @brief Synthesize a left mouse down+up at a point. */
void EventInjector::leftClick(CGPoint pos) {
    CGEventRef down = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseDown, pos, kCGMouseButtonLeft);
    CGEventRef up   = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseUp,   pos, kCGMouseButtonLeft);
    CGEventSetIntegerValueField(down, kCGMouseEventClickState, 1);
    CGEventSetIntegerValueField(up,   kCGMouseEventClickState, 1);
    CGEventPost(kCGHIDEventTap, down);
    struct timespec ts{0, 30'000'000};
    nanosleep(&ts, nullptr);
    CGEventPost(kCGHIDEventTap, up);
    CFRelease(down);
    CFRelease(up);
}

/** @brief Synthesize a right mouse down+up (context menu) at a point. */
void EventInjector::rightClick(CGPoint pos) {
    CGEventRef down = CGEventCreateMouseEvent(NULL, kCGEventRightMouseDown, pos, kCGMouseButtonRight);
    CGEventRef up   = CGEventCreateMouseEvent(NULL, kCGEventRightMouseUp,   pos, kCGMouseButtonRight);
    CGEventSetIntegerValueField(down, kCGMouseEventClickState, 1);
    CGEventSetIntegerValueField(up,   kCGMouseEventClickState, 1);
    CGEventPost(kCGHIDEventTap, down);
    struct timespec ts{0, 30'000'000};
    nanosleep(&ts, nullptr);
    CGEventPost(kCGHIDEventTap, up);
    CFRelease(down);
    CFRelease(up);
}

/** @brief Post a scroll-wheel event with the given line deltas. */
void EventInjector::scroll(int32_t deltaY, int32_t deltaX) {
    CGEventRef e = CGEventCreateScrollWheelEvent(
        NULL, kCGScrollEventUnitPixel, 2, deltaY, deltaX);
    CGEventPost(kCGHIDEventTap, e);
    CFRelease(e);
}

/** @brief Emit one zoom step as Cmd + / Cmd - depending on sign. */
void EventInjector::zoom(float magnification, int phase) {
    (void)phase;

    // Use Cmd+= (zoom in) or Cmd+- (zoom out) — works in Safari, Preview,
    if (magnification > 0) {
        // Cmd+= (zoom in)
        CGEventRef down = CGEventCreateKeyboardEvent(NULL, 24, true); // = key
        CGEventSetFlags(down, kCGEventFlagMaskCommand);
        CGEventPost(kCGHIDEventTap, down);
        CFRelease(down);

        struct timespec ts = {0, 30 * 1000 * 1000};
        nanosleep(&ts, NULL);

        CGEventRef up = CGEventCreateKeyboardEvent(NULL, 24, false);
        CGEventSetFlags(up, kCGEventFlagMaskCommand);
        CGEventPost(kCGHIDEventTap, up);
        CFRelease(up);
    } else {
        // Cmd+- (zoom out)
        CGEventRef down = CGEventCreateKeyboardEvent(NULL, 27, true); // - key
        CGEventSetFlags(down, kCGEventFlagMaskCommand);
        CGEventPost(kCGHIDEventTap, down);
        CFRelease(down);

        struct timespec ts = {0, 30 * 1000 * 1000};
        nanosleep(&ts, NULL);

        CGEventRef up = CGEventCreateKeyboardEvent(NULL, 27, false);
        CGEventSetFlags(up, kCGEventFlagMaskCommand);
        CGEventPost(kCGHIDEventTap, up);
        CFRelease(up);
    }
}
/** @brief Emit one rotate step as Cmd R / Cmd L depending on sign. */
void EventInjector::rotate(float direction, int phase) {
    (void)phase;
    if (direction == 0.0f) return;

    // One discrete rotation step. The gesture detector decides WHEN to fire
    // (accumulating wrist turn); this just emits a single Cmd+R (right) or
    // Cmd+L (left) — Preview's rotate-image shortcuts.
    CGKeyCode key = (direction > 0.0f) ? 15 /* R */ : 37 /* L */;
    CGEventRef down = CGEventCreateKeyboardEvent(NULL, key, true);
    CGEventSetFlags(down, kCGEventFlagMaskCommand);
    CGEventRef up   = CGEventCreateKeyboardEvent(NULL, key, false);
    CGEventPost(kCGHIDEventTap, down);
    CGEventPost(kCGHIDEventTap, up);
    CFRelease(down);
    CFRelease(up);
}

/** @brief Post a left multi-touch swipe (browser back). */
void EventInjector::swipeLeft() {
    CGEventRef keyDown = CGEventCreateKeyboardEvent(NULL, 33, true);
    CGEventSetFlags(keyDown, kCGEventFlagMaskCommand);
    CGEventPost(kCGHIDEventTap, keyDown);
    CFRelease(keyDown);

    struct timespec ts = {0, 30 * 1000 * 1000};
    nanosleep(&ts, NULL);

    CGEventRef keyUp = CGEventCreateKeyboardEvent(NULL, 33, false);
    CGEventSetFlags(keyUp, kCGEventFlagMaskCommand);
    CGEventPost(kCGHIDEventTap, keyUp);
    CFRelease(keyUp);
}

/** @brief Post a right multi-touch swipe (browser forward). */
void EventInjector::swipeRight() {
    CGEventRef keyDown = CGEventCreateKeyboardEvent(NULL, 30, true);
    CGEventSetFlags(keyDown, kCGEventFlagMaskCommand);
    CGEventPost(kCGHIDEventTap, keyDown);
    CFRelease(keyDown);

    struct timespec ts = {0, 30 * 1000 * 1000};
    nanosleep(&ts, NULL);

    CGEventRef keyUp = CGEventCreateKeyboardEvent(NULL, 30, false);
    CGEventSetFlags(keyUp, kCGEventFlagMaskCommand);
    CGEventPost(kCGHIDEventTap, keyUp);
    CFRelease(keyUp);
}

/** @brief Press and hold the left button (drag begin). */
void EventInjector::mouseDown(CGPoint pos) {
    CGEventRef e = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseDown, pos, kCGMouseButtonLeft);
    CGEventSetIntegerValueField(e, kCGMouseEventClickState, 1);
    CGEventPost(kCGHIDEventTap, e);
    CFRelease(e);
}

/** @brief Move with the left button held (drag continue). */
void EventInjector::mouseDragged(CGPoint pos) {
    CGEventRef e = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseDragged, pos, kCGMouseButtonLeft);
    CGEventSetIntegerValueField(e, kCGMouseEventClickState, 1);
    CGEventPost(kCGHIDEventTap, e);
    CFRelease(e);
}

/** @brief Release the left button (drag end / drop). */
void EventInjector::mouseUp(CGPoint pos) {
    CGEventRef e = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseUp, pos, kCGMouseButtonLeft);
    CGEventSetIntegerValueField(e, kCGMouseEventClickState, 1);
    CGEventPost(kCGHIDEventTap, e);
    CFRelease(e);
}

/** @brief Trigger a smart-zoom gesture at a point. */
void EventInjector::smartZoom(CGPoint pos) {
    for (int i = 0; i < 2; ++i) {
        CGEventRef down = CGEventCreateMouseEvent(
            NULL, kCGEventLeftMouseDown, pos, kCGMouseButtonLeft);
        CGEventRef up = CGEventCreateMouseEvent(
            NULL, kCGEventLeftMouseUp, pos, kCGMouseButtonLeft);

        CGEventSetIntegerValueField(down, kCGMouseEventClickState, 2);
        CGEventSetIntegerValueField(up, kCGMouseEventClickState, 2);

        CGEventPost(kCGHIDEventTap, down);
        CGEventPost(kCGHIDEventTap, up);

        CFRelease(down);
        CFRelease(up);
    }
}