#pragma once
#include "../core/Frame.h"
#include "../core/Hand.h"
#include <CoreGraphics/CoreGraphics.h>
#include <unordered_map>

// One of these classifies the current hand pose. Every frame the classifier
// picks exactly ONE pose, and only the detectors belonging to that pose run.
// That's what prevents scroll/zoom/rotate from all firing at once on the
// same two-finger motion.
enum class Pose {
    None,        // no fingers extended, hand open — ignored
    Fist,        // grab_strength high → drag / drop
    OneFinger,   // index only → cursor + tap-to-click
    TwoFinger,   // index + middle → scroll / right-click / swipe / smart-zoom
    Pinch        // thumb + index visible, changing distance → zoom / rotate
};

struct HandState {
    // ── Cursor smoothing ──────────────────────────────────────────────
    CGPoint smoothedPos = {0, 0};
    bool    hasPrevPos  = false;

    // ── Previous finger-tip positions (for velocity derivation) ───────
    Vector3 prevIndexTip  = {};
    Vector3 prevMiddleTip = {};
    Vector3 prevThumbTip  = {};
    bool    hasPrevTips   = false;

    // ── Two-finger gesture state ──────────────────────────────────────
    float prevIndexMiddleAngle = 0.0f;
    bool  hasPrevAngle         = false;

    // ── Pinch / zoom state ────────────────────────────────────────────
    float prevPinchDistance = -1.0f;

    // ── Tap edge detection (fingertip Z-velocity must return to ~0
    //    before a second tap is allowed — prevents one motion = many clicks)
    bool  oneFingerTapArmed = true;
    bool  twoFingerTapArmed = true;

    // ── Cooldowns ─────────────────────────────────────────────────────
    int tapCooldown       = 0;
    int swipeCooldown     = 0;
    int smartZoomCooldown = 0;

    // ── Swipe sustain (require velocity for N consecutive frames) ─────
    int swipeRightFrames  = 0;
    int swipeLeftFrames   = 0;

    // ── Smart zoom double-tap tracking ────────────────────────────────
    int lastTwoFingerTapFrame = -10000;

    // ── Drag / drop state ─────────────────────────────────────────────
    bool dragging            = false;
    int  grabRisingFrames    = 0;   // how many frames grab has been "on"
    int  grabFallingFrames   = 0;   // how many frames grab has been "off"
    CGPoint dragAnchor       = {0, 0};

    // ── Last (committed) pose and stability hysteresis ────────────────
    // Leap's is_extended flags flicker, so we only commit to a new pose
    // after seeing it for POSE_STABILITY_FRAMES consecutive raw frames.
    Pose lastPose           = Pose::None;
    Pose candidatePose      = Pose::None;
    int  candidateFrames    = 0;
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

    // ── Classifier ───────────────────────────────────────────────────
    Pose classify(const Hand& hand) const;

    // ── Helpers ──────────────────────────────────────────────────────
    CGPoint tipToScreen(const Vector3& p) const;
    CGPoint palmToScreen(const Vector3& p) const;
    float   indexMiddleAngle(const Hand& hand) const;

    // ── Per-pose handlers ────────────────────────────────────────────
    void handleOneFinger(const Hand& hand, HandState& state);
    void handleTwoFinger(const Hand& hand, HandState& state);
    void handlePinch    (const Hand& hand, HandState& state);
    void handleFist     (const Hand& hand, HandState& state);

    // ── Individual gesture detectors (called from handlers above) ────
    void moveCursorOneFinger (const Hand& hand, HandState& state);
    void moveCursorTwoFinger (const Hand& hand, HandState& state);
    bool detectOneFingerTap  (const Hand& hand, HandState& state);
    bool detectTwoFingerTap  (const Hand& hand, HandState& state);
    bool detectScroll        (const Hand& hand, HandState& state);
    bool detectSwipe         (const Hand& hand, HandState& state);
    bool detectZoom          (const Hand& hand, HandState& state);
    bool detectRotate        (const Hand& hand, HandState& state);

    // ── Bookkeeping ──────────────────────────────────────────────────
    void updatePrevTips(const Hand& hand, HandState& state);
    void resetTwoFingerState(HandState& state);
    void resetPinchState(HandState& state);
};