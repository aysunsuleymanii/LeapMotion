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

// ── Public implementation ────────────────────────────────────────────────────

bool EventInjector::checkAccessibility() {
    if (!AXIsProcessTrusted()) {
        std::cerr << "[LeapMotion] Accessibility permission required.\n"
                << "  Go to: System Settings → Privacy & Security → Accessibility\n"
                << "  Add this app, then re-launch.\n";
        return false;
    }
    return true;
}

void EventInjector::moveCursor(CGPoint pos) {
    CGWarpMouseCursorPosition(pos);
    CGEventRef e = CGEventCreateMouseEvent(NULL, kCGEventMouseMoved, pos, kCGMouseButtonLeft);
    CGEventPost(kCGHIDEventTap, e);
    CFRelease(e);
}

void EventInjector::leftClick(CGPoint pos) {
    CGEventRef down = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseDown, pos, kCGMouseButtonLeft);
    CGEventRef up   = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseUp,   pos, kCGMouseButtonLeft);
    // Many apps (especially OpenGL/Metal apps like Anatomy 3D Atlas) ignore
    // synthetic clicks that don't set the click state. This field tells the
    // OS "this is the 1st click in a click sequence."
    CGEventSetIntegerValueField(down, kCGMouseEventClickState, 1);
    CGEventSetIntegerValueField(up,   kCGMouseEventClickState, 1);
    CGEventPost(kCGHIDEventTap, down);
    // Small dwell between down and up so apps register a click instead of
    // interpreting "down + immediate up at slightly different coords" as
    // the start of a drag. 30ms is short enough to feel instant.
    struct timespec ts{0, 30'000'000};
    nanosleep(&ts, nullptr);
    CGEventPost(kCGHIDEventTap, up);
    CFRelease(down);
    CFRelease(up);
}

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

void EventInjector::scroll(int32_t deltaY, int32_t deltaX) {
    CGEventRef e = CGEventCreateScrollWheelEvent(
        NULL, kCGScrollEventUnitPixel, 2, deltaY, deltaX);
    CGEventPost(kCGHIDEventTap, e);
    CFRelease(e);
}

void EventInjector::zoom(float magnification, int phase) {
    (void)phase;

    // Use Cmd+= (zoom in) or Cmd+- (zoom out) — works in Safari, Preview,
    // Finder, and most macOS apps. Much more reliable than gesture events.
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
// Rotate has no universal macOS shortcut. Preview accepts Cmd+L / Cmd+R to
// rotate an image 90°, and that's good enough for a demo. We accumulate the
// incoming degree deltas in a static and only fire a shortcut once the
// threshold is crossed, so small per-frame deltas don't flood the system.
void EventInjector::rotate(float degrees, int phase) {
    (void)phase;

    static float accumulated = 0.0f;
    accumulated += degrees;

    // Fire a 90°-step rotate when accumulated delta crosses ±45°
    if (accumulated > 45.0f) {
        CGEventRef down = CGEventCreateKeyboardEvent(NULL, 15, true);  // R
        CGEventSetFlags(down, kCGEventFlagMaskCommand);
        CGEventRef up   = CGEventCreateKeyboardEvent(NULL, 15, false);
        CGEventPost(kCGHIDEventTap, down);
        CGEventPost(kCGHIDEventTap, up);
        CFRelease(down);
        CFRelease(up);
        accumulated = 0.0f;
    }
    else if (accumulated < -45.0f) {
        CGEventRef down = CGEventCreateKeyboardEvent(NULL, 37, true);  // L
        CGEventSetFlags(down, kCGEventFlagMaskCommand);
        CGEventRef up   = CGEventCreateKeyboardEvent(NULL, 37, false);
        CGEventPost(kCGHIDEventTap, down);
        CGEventPost(kCGHIDEventTap, up);
        CFRelease(down);
        CFRelease(up);
        accumulated = 0.0f;
    }
}

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

void EventInjector::mouseDown(CGPoint pos) {
    CGEventRef e = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseDown, pos, kCGMouseButtonLeft);
    CGEventSetIntegerValueField(e, kCGMouseEventClickState, 1);
    CGEventPost(kCGHIDEventTap, e);
    CFRelease(e);
}

void EventInjector::mouseDragged(CGPoint pos) {
    CGEventRef e = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseDragged, pos, kCGMouseButtonLeft);
    CGEventSetIntegerValueField(e, kCGMouseEventClickState, 1);
    CGEventPost(kCGHIDEventTap, e);
    CFRelease(e);
}

void EventInjector::mouseUp(CGPoint pos) {
    CGEventRef e = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseUp, pos, kCGMouseButtonLeft);
    CGEventSetIntegerValueField(e, kCGMouseEventClickState, 1);
    CGEventPost(kCGHIDEventTap, e);
    CFRelease(e);
}

void EventInjector::smartZoom(CGPoint pos) {
    // macOS smart zoom = Option + double-tap gesture
    // Simulated via Cmd+Shift+H or a magnification event with phase
    // Best approximation: send a double left-click at position
    // which triggers smart zoom in most macOS apps (Safari, Preview, Maps)

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