#pragma once
#include "../core/Frame.h"
#include "../core/Hand.h"
#include <CoreGraphics/CoreGraphics.h>
#include <unordered_map>


enum class Pose {
    None,        // no fingers extended, hand open — ignored
    Fist,        // grab_strength high → drag / drop
    OneFinger,   // index only → cursor + tap-to-click
    TwoFinger,   // index + middle → scroll / right-click / swipe / smart-zoom
    Pinch        // thumb + index visible, changing distance → zoom / rotate
};

struct HandState {
    CGPoint smoothedPos = {0, 0};
    bool    hasPrevPos  = false;
    CGPoint stablePos   = {0, 0};
    bool    hasStablePos = false;

    Vector3 prevIndexTip  = {};
    Vector3 prevMiddleTip = {};
    Vector3 prevThumbTip  = {};
    bool    hasPrevTips   = false;

    float prevIndexMiddleAngle = 0.0f;
    bool  hasPrevAngle         = false;

    float prevPinchDistance = -1.0f;
    bool  oneFingerTapArmed = true;
    bool  twoFingerTapArmed = true;

    int tapCooldown       = 0;
    int swipeCooldown     = 0;
    int smartZoomCooldown = 0;

    int swipeRightFrames  = 0;
    int swipeLeftFrames   = 0;

    int lastTwoFingerTapFrame = -10000;

    bool    dragging            = false;
    int     grabRisingFrames    = 0;   // how many frames grab has been "on"
    int     grabFallingFrames   = 0;   // how many frames grab has been "off"
    CGPoint dragAnchor          = {0, 0};
    Vector3 dragPalmStart       = {};
    bool    hasDragPalmStart    = false;

    Pose lastPose           = Pose::None;
    Pose candidatePose      = Pose::None;
    int  candidateFrames    = 0;
    int lastScrollFrame = -100;
    int postDragScrollCooldown = 0;
    int lastPinchFrame = -100;
    int lastZoomFrame = -100;
};

class GestureDetector {
public:
    GestureDetector();
    void update(const Frame& frame);

private:
    float screenW_     = 0;
    float screenH_     = 0;
    int   frameCount_  = 0;

    std::unordered_map<uint32_t, HandState> states_;

    Pose classify(const Hand& hand) const;

    CGPoint tipToScreen(const Vector3& p) const;
    CGPoint palmToScreen(const Vector3& p) const;
    float   indexMiddleAngle(const Hand& hand) const;

    void handleOneFinger(const Hand& hand, HandState& state);
    void handleTwoFinger(const Hand& hand, HandState& state);
    void handlePinch    (const Hand& hand, HandState& state);
    void handleFist     (const Hand& hand, HandState& state);

    void moveCursorOneFinger (const Hand& hand, HandState& state);
    void moveCursorTwoFinger (const Hand& hand, HandState& state);
    bool detectOneFingerTap  (const Hand& hand, HandState& state);
    bool detectTwoFingerTap  (const Hand& hand, HandState& state);
    bool detectScroll        (const Hand& hand, HandState& state);
    bool detectSwipe         (const Hand& hand, HandState& state);
    bool detectZoom          (const Hand& hand, HandState& state);
    bool detectRotate        (const Hand& hand, HandState& state);

    void updatePrevTips(const Hand& hand, HandState& state);
    void resetTwoFingerState(HandState& state, int currentFrame);
    void resetPinchState(HandState& state);
};