#pragma once

#include <CoreGraphics/CoreGraphics.h>

class EventInjector {
public:
    static bool checkAccessibility();

    static void moveCursor(CGPoint pos);

    static void leftClick(CGPoint pos);
    static void rightClick(CGPoint pos);

    static void scroll(int32_t deltaY, int32_t deltaX = 0);

    static void zoom(float magnification, int phase);

    static void rotate(float degrees, int phase);

    static void swipeLeft();
    static void swipeRight();

    static void mouseDown(CGPoint pos);
    static void mouseDragged(CGPoint pos);
    static void mouseUp(CGPoint pos);
    static void smartZoom(CGPoint pos);
};