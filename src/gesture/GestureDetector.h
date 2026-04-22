#pragma once
#include "../core/Frame.h"
#include "../core/Hand.h"
#include <CoreGraphics/CoreGraphics.h>
#include <unordered_map>

struct HandState
{
    // ── Cursor smoothing ──
    CGPoint smoothedPos        = {0, 0};
    CGPoint prevRawPos         = {0, 0};
    bool    hasPrevPos         = false;

    // ── Two-finger gesture state ──
    CGPoint prevTwoFingerCentroid = {-1, -1};
    float   prevTwoFingerDist     = -1.0f;
    float   prevTwoFingerAngle    = 0.0f;

    // ── Pinch history ──
    float prevPinch            = 0.0f;
    Vector3 prevPalmPos        = {};

    // ── Cooldowns ──
    int tapCooldown            = 0;
    int swipeCooldown          = 0;
    int smartZoomCooldown      = 0;

    // ── Smart zoom double-tap ──
    int lastPinchFrame         = 0;

    // ── Gesture flag (prevents multiple gestures per frame) ──
    bool gestureActive         = false;

    // ── Drag state (kept for future grab support) ──
    bool    dragging           = false;
    float   smoothedGrab       = 0.0f;
    int     grabStableFrames   = 0;
};

class GestureDetector
{
public:
    GestureDetector();
    void update(const Frame& frame);

private:
    // ── Screen dimensions ──
    float screenW_ = 0;
    float screenH_ = 0;
    int   frameCount_ = 0;

    // ── Per-hand state ──
    std::unordered_map<uint32_t, HandState> states_;

    // ── Helpers ──
    CGPoint tipToScreen(const Vector3& p) const;
    float   twoFingerDistance(const Hand& hand) const;
    float   twoFingerAngle(const Hand& hand) const;
    CGPoint twoFingerCentroid(const Hand& hand) const;

    // ── Gesture detectors ──
    void detectAirCursor       (const Hand& hand, HandState& state);
    void detectTwoFingerCursor (const Hand& hand, HandState& state);
    void detectTapClick        (const Hand& hand, HandState& state);
    void detectRightClick      (const Hand& hand, HandState& state);
    void detectScroll          (const Hand& hand, HandState& state);
    void detectZoom            (const Hand& hand, HandState& state);
    void detectRotate          (const Hand& hand, HandState& state);
    void detectSwipe           (const Hand& hand, HandState& state);
    void detectSmartZoom       (const Hand& hand, HandState& state);
};