#pragma once

#include <CoreGraphics/CoreGraphics.h>

class EventInjector {
public:
    // Call once at startup — checks Accessibility permission
    static bool checkAccessibility();

    // ── Cursor ──────────────────────────────────────────────────────────────
    static void moveCursor(CGPoint pos);

    // ── Click ───────────────────────────────────────────────────────────────
    static void leftClick(CGPoint pos);
    static void rightClick(CGPoint pos);

    // ── Scroll ──────────────────────────────────────────────────────────────
    // deltaY: positive = scroll up, deltaX: positive = scroll right
    static void scroll(int32_t deltaY, int32_t deltaX = 0);

    // ── Pinch / Zoom ────────────────────────────────────────────────────────
    // phase: 0=begin 1=changed 2=end
    static void zoom(float magnification, int phase);

    // ── Rotate ──────────────────────────────────────────────────────────────
    // rotation in degrees, phase: 0=begin 1=changed 2=end
    static void rotate(float degrees, int phase);

    // ── Swipe ───────────────────────────────────────────────────────────────
    static void swipeLeft();
    static void swipeRight();

    // ── Drag ────────────────────────────────────────────────────────────────
    static void mouseDown(CGPoint pos);
    static void mouseDragged(CGPoint pos);
    static void mouseUp(CGPoint pos);
    static void smartZoom(CGPoint pos);
};