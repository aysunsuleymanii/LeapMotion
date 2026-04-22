#include "EventInjector.h"
#include <ApplicationServices/ApplicationServices.h>
#include <iostream>


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
    CGEventRef up = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseUp, pos, kCGMouseButtonLeft);
    CGEventPost(kCGHIDEventTap, down);
    CGEventPost(kCGHIDEventTap, up);
    CFRelease(down);
    CFRelease(up);
}

void EventInjector::rightClick(CGPoint pos) {
    CGEventRef down = CGEventCreateMouseEvent(NULL, kCGEventRightMouseDown, pos, kCGMouseButtonRight);
    CGEventRef up = CGEventCreateMouseEvent(NULL, kCGEventRightMouseUp, pos, kCGMouseButtonRight);
    CGEventPost(kCGHIDEventTap, down);
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
    (void)phase;  // not used — macOS Cmd+scroll has no gesture-phase concept

    // Cmd + scroll = zoom in most macOS apps (Safari, Preview, Finder, Maps).
    // Positive magnification (fingers spreading apart) → scroll up → zoom in.
    int scrollAmount = static_cast<int>(magnification * 10);
    if (scrollAmount == 0) return;   // nothing worth sending

    CGEventRef scroll = CGEventCreateScrollWheelEvent(
        NULL,
        kCGScrollEventUnitLine,   // "line" units register as zoom steps
        1,
        scrollAmount
    );
    CGEventSetFlags(scroll, kCGEventFlagMaskCommand);
    CGEventPost(kCGHIDEventTap, scroll);
    CFRelease(scroll);
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
    // Cmd + [  (Back)
    CGEventRef keyDown = CGEventCreateKeyboardEvent(NULL, 33, true); // [
    CGEventSetFlags(keyDown, kCGEventFlagMaskCommand);
    CGEventPost(kCGHIDEventTap, keyDown);

    CGEventRef keyUp = CGEventCreateKeyboardEvent(NULL, 33, false);
    CGEventPost(kCGHIDEventTap, keyUp);

    CFRelease(keyDown);
    CFRelease(keyUp);
}

void EventInjector::swipeRight() {
    // Cmd + ]  (Forward)
    CGEventRef keyDown = CGEventCreateKeyboardEvent(NULL, 30, true); // ]
    CGEventSetFlags(keyDown, kCGEventFlagMaskCommand);
    CGEventPost(kCGHIDEventTap, keyDown);

    CGEventRef keyUp = CGEventCreateKeyboardEvent(NULL, 30, false);
    CGEventPost(kCGHIDEventTap, keyUp);

    CFRelease(keyDown);
    CFRelease(keyUp);
}

void EventInjector::mouseDown(CGPoint pos) {
    CGEventRef e = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseDown, pos, kCGMouseButtonLeft);
    CGEventPost(kCGHIDEventTap, e);
    CFRelease(e);
}

void EventInjector::mouseDragged(CGPoint pos) {
    CGEventRef e = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseDragged, pos, kCGMouseButtonLeft);
    CGEventPost(kCGHIDEventTap, e);
    CFRelease(e);
}

void EventInjector::mouseUp(CGPoint pos) {
    CGEventRef e = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseUp, pos, kCGMouseButtonLeft);
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