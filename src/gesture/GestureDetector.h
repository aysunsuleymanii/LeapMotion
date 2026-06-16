#pragma once
#include "../core/Frame.h"
#include "../core/Hand.h"
#include <CoreGraphics/CoreGraphics.h>
#include <unordered_map>

/**
 * @file GestureDetector.h
 * @brief Pose classification and gesture recognition — the heart of the app.
 */

/**
 * @enum Pose
 * @brief The coarse hand shape a frame is classified into.
 *
 * Exactly one pose is active per hand at a time. Each pose owns a family of
 * gestures, which keeps unrelated gestures from interfering (e.g. zoom lives on
 * Pinch, rotate on OpenHand, so they can never fight each other).
 */
enum class Pose {
    None,        ///< No fingers extended / hand open and idle — ignored.
    Fist,        ///< High grab strength → drag / drop.
    OneFinger,   ///< Index only → cursor movement + tap-to-click.
    TwoFinger,   ///< Index + middle → scroll / right-click / swipe / smart-zoom.
    Pinch,       ///< Thumb + index, changing distance → zoom.
    OpenHand     ///< All fingers extended, turned like a dial → rotate.
};

/**
 * @struct HandState
 * @brief Per-hand memory carried between frames.
 *
 * Gesture recognition is stateful: smoothing, debouncing, tap arming, swipe
 * sustain counters, the zoom accumulator, the rotate baseline, drag bookkeeping
 * and the pose-stability machine all need to remember the previous frame(s).
 * One HandState exists per tracked hand id (see GestureDetector::states_).
 */
struct HandState {
    CGPoint smoothedPos = {0, 0};   ///< Low-pass-filtered cursor position.
    bool    hasPrevPos  = false;    ///< Whether smoothedPos is seeded.
    CGPoint stablePos   = {0, 0};   ///< Cursor frozen at gesture start (for clicks/zoom).
    bool    hasStablePos = false;   ///< Whether stablePos is set.

    Vector3 prevIndexTip  = {};     ///< Last index tip (tap velocity).
    Vector3 prevMiddleTip = {};     ///< Last middle tip.
    Vector3 prevThumbTip  = {};     ///< Last thumb tip.
    bool    hasPrevTips   = false;  ///< Whether the prev* tips are seeded.

    float prevIndexMiddleAngle = 0.0f;  ///< Previous rotate angle (radians).
    bool  hasPrevAngle         = false; ///< Whether the rotate baseline is set.
    float rotAccumDeg          = 0.0f;  ///< Accumulated wrist turn (degrees).

    float prevPinchDistance = -1.0f;    ///< Previous thumb–index distance (mm); <0 = unseeded.
    float prevPinchStrength = -1.0f;    ///< Previous pinch strength.
    float zoomAccum         = 0.0f;     ///< Accumulated pinch-distance change toward a zoom step.
    bool  zoomedOutThisPinch = false;   ///< User actively squeezed to zoom out this pinch.
    bool  zoomedInThisPinch  = false;   ///< User actively spread to zoom in this pinch.
    float pinchPeak         = 0.0f;     ///< (legacy) peak pinch strength.
    bool  zoomReleasing     = false;    ///< (legacy) release latch.
    bool  oneFingerTapArmed = true;     ///< Index tap is ready to fire.
    bool  twoFingerTapArmed = true;     ///< Two-finger tap is ready to fire.

    int tapCooldown       = 0;          ///< Frames to suppress taps after one fires.
    int swipeCooldown     = 0;          ///< Frames to suppress swipes after one fires.
    int smartZoomCooldown = 0;          ///< Frames to suppress smart-zoom.

    int swipeRightFrames  = 0;          ///< Consecutive fast-right frames (sustain).
    int swipeLeftFrames   = 0;          ///< Consecutive fast-left frames (sustain).

    int lastTwoFingerTapFrame = -10000; ///< Frame of last two-finger tap (double-tap timing).

    bool    dragging            = false;///< A drag is currently in progress.
    int     grabRisingFrames    = 0;    ///< Frames grab has been continuously "on".
    int     grabFallingFrames   = 0;    ///< Frames grab has been continuously "off".
    CGPoint dragAnchor          = {0, 0};///< Screen anchor where the drag began.
    Vector3 dragPalmStart       = {};   ///< Palm position where the drag began.
    bool    hasDragPalmStart    = false;///< Whether dragPalmStart is set.

    Pose lastPose           = Pose::None;///< Currently committed pose.
    Pose candidatePose      = Pose::None;///< Pose proposed by the latest frame.
    int  candidateFrames    = 0;        ///< Frames the candidate has persisted.
    int lastScrollFrame = -100;         ///< Frame of last scroll (debounce).
    int postDragScrollCooldown = 0;     ///< Suppress scroll right after a drop.
    int lastPinchFrame = -100;          ///< Frame the last pinch ended.
    int lastZoomFrame = -100;           ///< Frame of last zoom step (throttle).
};

/**
 * @class GestureDetector
 * @brief Classifies each frame's pose and emits the matching system gesture.
 *
 * Pipeline per update():
 *  1. classify() reduces the hand to a Pose.
 *  2. A stability filter (POSE_STABILITY_FRAMES) debounces pose switches.
 *  3. The committed pose is dispatched to its handler (handlePinch, etc.).
 *  4. Handlers call the detect*() helpers, which post events via EventInjector.
 *
 * State for every hand is kept in @c states_, keyed by hand id, so two hands are
 * tracked independently.
 */
class GestureDetector {
public:
    /** @brief Construct and cache the screen dimensions. */
    GestureDetector();
    /**
     * @brief Process one tracking frame: classify pose and fire gestures.
     * @param frame The latest frame from LeapAdapter.
     */
    void update(const Frame& frame);

private:
    float screenW_     = 0;   ///< Cached display width (px).
    float screenH_     = 0;   ///< Cached display height (px).
    int   frameCount_  = 0;   ///< Monotonic processed-frame counter.

    std::unordered_map<uint32_t, HandState> states_;  ///< Per-hand state by id.

    /** @brief Reduce a hand to a Pose from finger/pinch/grab features. */
    Pose classify(const Hand& hand) const;

    /** @brief Map a fingertip position to absolute screen coordinates. */
    CGPoint tipToScreen(const Vector3& p) const;
    /** @brief Map a palm position to absolute screen coordinates. */
    CGPoint palmToScreen(const Vector3& p) const;
    /** @brief Angle of the index→middle vector (legacy rotate helper). */
    float   indexMiddleAngle(const Hand& hand) const;

    /** @brief OneFinger pose: cursor movement + tap-to-click. */
    void handleOneFinger(const Hand& hand, HandState& state);
    /** @brief TwoFinger pose: scroll / swipe / right-click / smart-zoom. */
    void handleTwoFinger(const Hand& hand, HandState& state);
    /** @brief Pinch pose: zoom in/out (incl. hold-to-continue). */
    void handlePinch    (const Hand& hand, HandState& state);
    /** @brief Fist pose: drag and drop. */
    void handleFist     (const Hand& hand, HandState& state);
    /** @brief OpenHand pose: rotate (turn the flat hand like a dial). */
    void handleOpenHand (const Hand& hand, HandState& state);

    /** @brief Smooth + map the index tip to the cursor. */
    void moveCursorOneFinger (const Hand& hand, HandState& state);
    /** @brief Smooth + map the two-finger midpoint to the cursor. */
    void moveCursorTwoFinger (const Hand& hand, HandState& state);
    /** @brief Detect a downward index tap → left click. @return true if fired. */
    bool detectOneFingerTap  (const Hand& hand, HandState& state);
    /** @brief Detect a two-finger tap → right click / smart-zoom. @return true if fired. */
    bool detectTwoFingerTap  (const Hand& hand, HandState& state);
    /** @brief Detect forward/back palm motion → scroll. @return true if fired. */
    bool detectScroll        (const Hand& hand, HandState& state);
    /** @brief Detect a fast horizontal flick → swipe. @return true if fired. */
    bool detectSwipe         (const Hand& hand, HandState& state);
    /** @brief Detect spread/squeeze → zoom in/out. @return true if a step fired. */
    bool detectZoom          (const Hand& hand, HandState& state);
    /** @brief Detect wrist turn of the open hand → rotate. @return true if a step fired. */
    bool detectRotate        (const Hand& hand, HandState& state);

    /** @brief Store this frame's tips for next-frame velocity maths. */
    void updatePrevTips(const Hand& hand, HandState& state);
    /** @brief Clear TwoFinger transient counters on pose exit. */
    void resetTwoFingerState(HandState& state, int currentFrame);
    /** @brief Clear pinch/zoom/rotate baselines on pose exit. */
    void resetPinchState(HandState& state);
};