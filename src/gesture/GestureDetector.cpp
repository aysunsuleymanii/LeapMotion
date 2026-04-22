#include "GestureDetector.h"
#include "../output/EventInjector.h"
#include "../core/Config.h"
#include <iostream>
#include <cmath>

// ── Constructor ─────────────────────────────────────────

GestureDetector::GestureDetector()
{
    CGDirectDisplayID display = CGMainDisplayID();
    screenW_ = static_cast<float>(CGDisplayPixelsWide(display));
    screenH_ = static_cast<float>(CGDisplayPixelsHigh(display));
}

// ── Main update ─────────────────────────────────────────

void GestureDetector::update(const Frame& frame)
{
    ++frameCount_;

    for (const auto& hand : frame.hands())
    {
        HandState& state = states_[hand.id()];
        state.gestureActive = false;

        // Cooldowns
        if (state.tapCooldown       > 0) --state.tapCooldown;
        if (state.swipeCooldown     > 0) --state.swipeCooldown;
        if (state.smartZoomCooldown > 0) --state.smartZoomCooldown;

        int extended = hand.extendedFingerCount();

        // ── 1 finger: cursor + tap-to-click ──
        if (extended == 1)
        {
            detectAirCursor(hand, state);
            detectTapClick(hand, state);
            state.prevTwoFingerDist  = -1.0f;
            state.prevTwoFingerAngle = 0.0f;
            state.prevTwoFingerCentroid = {-1, -1};
        }
        // ── 2 fingers: all trackpad gestures ──
        else if (extended == 2)
        {
            // Cursor follows centroid of index + middle fingertips
            detectTwoFingerCursor(hand, state);

            if (!state.gestureActive) detectRightClick(hand, state);
            if (!state.gestureActive) detectScroll(hand, state);
            if (!state.gestureActive) detectZoom(hand, state);
            if (!state.gestureActive) detectRotate(hand, state);
            if (!state.gestureActive) detectSwipe(hand, state);
            if (!state.gestureActive) detectSmartZoom(hand, state);
        }
        else
        {
            // Reset two-finger state when finger count changes
            state.prevTwoFingerDist     = -1.0f;
            state.prevTwoFingerAngle    = 0.0f;
            state.prevTwoFingerCentroid = {-1, -1};
        }

        // Save previous pinch for edge detection
        state.prevPinch    = hand.pinch();
        state.prevPalmPos  = hand.palmPosition();
    }
}

// ── Helpers ─────────────────────────────────────────────

CGPoint GestureDetector::tipToScreen(const Vector3& p) const
{
    float sx = (p.x * Config::CURSOR_SCALE_X) + screenW_ * 0.5f;
    float sy = (-p.y * Config::CURSOR_SCALE_Y) + screenH_ * 0.5f;
    sx = std::max(0.0f, std::min(sx, screenW_ - 1));
    sy = std::max(0.0f, std::min(sy, screenH_ - 1));
    return CGPointMake(sx, sy);
}

// Distance between index and middle fingertips
float GestureDetector::twoFingerDistance(const Hand& hand) const
{
    return hand.fingerTip(1).distanceTo(hand.fingerTip(2));
}

// Angle of vector from index tip to middle tip
float GestureDetector::twoFingerAngle(const Hand& hand) const
{
    Vector3 v = hand.fingerTip(2) - hand.fingerTip(1);
    return v.xyAngle();
}

// Midpoint between index and middle fingertips in screen space
CGPoint GestureDetector::twoFingerCentroid(const Hand& hand) const
{
    Vector3 mid;
    mid.x = (hand.fingerTip(1).x + hand.fingerTip(2).x) * 0.5f;
    mid.y = (hand.fingerTip(1).y + hand.fingerTip(2).y) * 0.5f;
    mid.z = (hand.fingerTip(1).z + hand.fingerTip(2).z) * 0.5f;
    return tipToScreen(mid);
}

// ── Air Cursor (1 finger — index tip) ───────────────────

void GestureDetector::detectAirCursor(const Hand& hand, HandState& state)
{
    if (state.gestureActive) return;

    CGPoint raw = tipToScreen(hand.fingerTip(1));

    if (!state.hasPrevPos)
    {
        state.smoothedPos = raw;
        state.prevRawPos  = raw;
        state.hasPrevPos  = true;
        return;
    }

    float dx   = raw.x - state.smoothedPos.x;
    float dy   = raw.y - state.smoothedPos.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    // ── Outlier rejection ──
    // If the raw position jumps more than 180px in one frame,
    // it's a tracking glitch — ignore it completely
    if (dist > 180.0f)
    {
        EventInjector::moveCursor(state.smoothedPos);
        return;
    }

    // ── Dead zone ──
    // Sub-3px movement is just hand tremor — don't move the cursor
    if (dist < 3.0f)
    {
        EventInjector::moveCursor(state.smoothedPos);
        return;
    }

    // ── Adaptive smoothing ──
    // Slow movement (dist < 10px): alpha ~0.25 — heavily smoothed, no shake
    // Fast movement (dist > 60px): alpha ~0.85 — nearly raw, responsive
    float alpha = 0.25f + std::min(dist / 70.0f, 0.60f);

    state.smoothedPos.x = state.smoothedPos.x * (1.0f - alpha) + raw.x * alpha;
    state.smoothedPos.y = state.smoothedPos.y * (1.0f - alpha) + raw.y * alpha;

    EventInjector::moveCursor(state.smoothedPos);
    state.prevRawPos = raw;
}


// ── Two-finger cursor (centroid of index + middle) ───────
// Replace your existing detectTwoFingerCursor with this

void GestureDetector::detectTwoFingerCursor(const Hand& hand, HandState& state)
{
    CGPoint raw = twoFingerCentroid(hand);

    if (state.prevTwoFingerCentroid.x < 0)
    {
        state.prevTwoFingerCentroid = raw;
        state.smoothedPos           = raw;
        return;
    }

    float dx   = raw.x - state.smoothedPos.x;
    float dy   = raw.y - state.smoothedPos.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    // Same outlier rejection — centroid can also spike
    if (dist > 180.0f)
    {
        EventInjector::moveCursor(state.smoothedPos);
        return;
    }

    if (dist < 3.0f)
    {
        EventInjector::moveCursor(state.smoothedPos);
        return;
    }

    float alpha = 0.25f + std::min(dist / 70.0f, 0.60f);

    state.smoothedPos.x = state.smoothedPos.x * (1.0f - alpha) + raw.x * alpha;
    state.smoothedPos.y = state.smoothedPos.y * (1.0f - alpha) + raw.y * alpha;

    EventInjector::moveCursor(state.smoothedPos);
    state.prevTwoFingerCentroid = raw;
}

void GestureDetector::detectTapClick(const Hand& hand, HandState& state)
{
    if (state.gestureActive || state.tapCooldown > 0) return;

    if (hand.pinch() > Config::TAP_PINCH_ON &&
        state.prevPinch < Config::TAP_PINCH_OFF)
    {
        EventInjector::leftClick(state.smoothedPos);
        state.tapCooldown  = Config::TAP_COOLDOWN_FRAMES;
        state.gestureActive = true;
    }
}

// ── Right Click (2-finger tap) ───────────────────────────

void GestureDetector::detectRightClick(const Hand& hand, HandState& state)
{
    if (state.gestureActive || state.tapCooldown > 0) return;

    // Rising edge on pinch with 2 fingers = right click
    if (hand.pinch() > Config::TAP_PINCH_ON &&
        state.prevPinch < Config::TAP_PINCH_OFF)
    {
        EventInjector::rightClick(state.smoothedPos);
        state.tapCooldown  = Config::TAP_COOLDOWN_FRAMES;
        state.gestureActive = true;
    }
}

// ── Scroll (2 fingers slide up/down or left/right) ───────

void GestureDetector::detectScroll(const Hand& hand, HandState& state)
{
    if (state.gestureActive) return;

    // Use palm velocity as proxy for two-finger slide velocity
    float vy = hand.palmVelocity().y;
    float vx = hand.palmVelocity().x;

    if (std::abs(vy) < Config::SCROLL_MIN_SPEED &&
        std::abs(vx) < Config::SCROLL_MIN_SPEED)
        return;

    // Suppress scroll if pinch is active (user intending to tap/zoom)
    if (hand.pinch() > 0.5f) return;

    EventInjector::scroll(
        static_cast<int32_t>(vy * Config::SCROLL_SENSITIVITY),
        static_cast<int32_t>(-vx * Config::SCROLL_SENSITIVITY)
    );

    state.gestureActive = true;
}

// ── Zoom (pinch in/out with index + middle) ──────────────

void GestureDetector::detectZoom(const Hand& hand, HandState& state)
{
    if (state.gestureActive) return;

    float dist = twoFingerDistance(hand);

    if (state.prevTwoFingerDist < 0.0f)
    {
        state.prevTwoFingerDist = dist;
        return;
    }

    float delta = dist - state.prevTwoFingerDist;

    // Noise floor
    if (std::abs(delta) < 1.5f) return;

    // Positive delta = fingers apart = zoom in
    // Negative delta = fingers together = zoom out
    EventInjector::zoom(delta * Config::PINCH_SCALE, 2);
    state.prevTwoFingerDist = dist;
    state.gestureActive     = true;
}

// ── Rotate (2 fingers rotate around each other) ──────────

void GestureDetector::detectRotate(const Hand& hand, HandState& state)
{
    if (state.gestureActive) return;

    float angle = twoFingerAngle(hand);
    float delta = angle - state.prevTwoFingerAngle;

    // Wrap-around guard
    if (delta >  M_PI) delta -= 2.0f * M_PI;
    if (delta < -M_PI) delta += 2.0f * M_PI;

    // Jitter filter
    if (std::abs(delta) < 0.02f)
    {
        state.prevTwoFingerAngle = angle;
        return;
    }

    EventInjector::rotate(delta * Config::ROTATE_SCALE, 2);
    state.prevTwoFingerAngle = angle;
    state.gestureActive      = true;
}

// ── Swipe (2 fingers fast left/right) ────────────────────

void GestureDetector::detectSwipe(const Hand& hand, HandState& state)
{
    if (state.gestureActive || state.swipeCooldown > 0) return;

    // Suppress if pinch active
    if (hand.pinch() > 0.5f) return;

    float vx = hand.palmVelocity().x;

    if (vx > Config::SWIPE_VELOCITY_THRESHOLD)
    {
        EventInjector::swipeRight();
        state.swipeCooldown = Config::SWIPE_COOLDOWN_FRAMES;
        state.gestureActive = true;
    }
    else if (vx < -Config::SWIPE_VELOCITY_THRESHOLD)
    {
        EventInjector::swipeLeft();
        state.swipeCooldown = Config::SWIPE_COOLDOWN_FRAMES;
        state.gestureActive = true;
    }
}

// ── Smart Zoom (2-finger double tap) ─────────────────────

void GestureDetector::detectSmartZoom(const Hand& hand, HandState& state)
{
    if (state.smartZoomCooldown > 0) return;

    bool risingEdge =
        (hand.pinch() > Config::TAP_PINCH_ON &&
         state.prevPinch < Config::TAP_PINCH_OFF);

    if (!risingEdge) return;

    int gap = frameCount_ - state.lastPinchFrame;

    if (state.lastPinchFrame > 0 &&
        gap <= Config::SMART_ZOOM_MAX_GAP_FRAMES)
    {
        // Double tap detected → smart zoom
        EventInjector::smartZoom(state.smoothedPos);
        state.smartZoomCooldown = 45;
        state.lastPinchFrame    = -999;
    }
    else
    {
        state.lastPinchFrame = frameCount_;
    }
}