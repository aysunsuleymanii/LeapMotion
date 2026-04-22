#include "GestureDetector.h"
#include "../output/EventInjector.h"
#include "../core/Config.h"
#include <cmath>
#include <algorithm>
#include <iostream>

// Helper: return the short name for a Pose (for debug logging)
static const char* poseName(Pose p) {
    switch (p) {
        case Pose::None:      return "None";
        case Pose::Fist:      return "Fist";
        case Pose::OneFinger: return "OneFinger";
        case Pose::TwoFinger: return "TwoFinger";
        case Pose::Pinch:     return "Pinch";
    }
    return "?";
}

// ─────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────

GestureDetector::GestureDetector()
{
    CGDirectDisplayID display = CGMainDisplayID();
    screenW_ = static_cast<float>(CGDisplayPixelsWide(display));
    screenH_ = static_cast<float>(CGDisplayPixelsHigh(display));
}

// ─────────────────────────────────────────────────────────────────────────
// Main update loop. For each hand we:
//   1. Classify the pose (fist / one-finger / two-finger / pinch / none).
//   2. If the pose changed, reset the previous pose's per-frame state so
//      stale values (e.g. last pinch distance) don't leak into the new pose.
//   3. Dispatch to exactly ONE handler — never multiple in parallel.
//   4. Store previous-frame values for velocity/edge detection next frame.
// ─────────────────────────────────────────────────────────────────────────

void GestureDetector::update(const Frame& frame)
{
    ++frameCount_;

    for (const auto& hand : frame.hands())
    {
        HandState& state = states_[hand.id()];

        // Decrement cooldowns once per frame
        if (state.tapCooldown       > 0) --state.tapCooldown;
        if (state.swipeCooldown     > 0) --state.swipeCooldown;
        if (state.smartZoomCooldown > 0) --state.smartZoomCooldown;

        // The drag gesture is pose-independent: a grab that starts as a
        // fist must continue dragging even if the user transiently opens
        // a finger. So we update drag state FIRST, based on grab_strength
        // alone, and only if we're not dragging do we classify a fresh pose.
        if (state.dragging)
        {
            handleFist(hand, state);
        }
        else
        {
            // ── Pose hysteresis ─────────────────────────────────────
            // classify() looks at the current frame only. But Leap's flags
            // flicker, so we apply a stability window: a new raw pose must
            // appear for N consecutive frames before it becomes the "committed"
            // pose that handlers actually see. This kills cursor jumps and
            // spurious clicks caused by 1-2 frame thumb flickers.
            Pose raw = classify(hand);
            if (raw == state.candidatePose)
            {
                if (state.candidateFrames < Config::POSE_STABILITY_FRAMES)
                    ++state.candidateFrames;
            }
            else
            {
                state.candidatePose   = raw;
                state.candidateFrames = 1;
            }

            // Only commit the new pose once it's been stable long enough.
            // Before that, we keep running the PREVIOUS committed pose's
            // handler — which is the whole point, since the old pose is
            // the one that actually matches what the user is still doing.
            Pose pose = state.lastPose;
            if (state.candidateFrames >= Config::POSE_STABILITY_FRAMES)
                pose = state.candidatePose;

            // Reset per-pose state when the committed pose changes so e.g. the
            // old pinch-distance doesn't get interpreted as "change" on the
            // first frame of a new pinch.
            if (pose != state.lastPose)
            {
                if (Config::DEBUG_GESTURES)
                    std::cerr << "[pose] " << poseName(state.lastPose)
                              << " -> " << poseName(pose) << "\n";

                if (state.lastPose == Pose::TwoFinger) resetTwoFingerState(state);
                if (state.lastPose == Pose::Pinch)    resetPinchState(state);
                // Drop stale cursor/tip history — it's from a different pose
                // whose coordinates don't correspond to the new one's.
                state.hasPrevPos  = false;
                state.hasPrevTips = false;
                if (pose == Pose::OneFinger) state.oneFingerTapArmed = true;
                if (pose == Pose::TwoFinger) state.twoFingerTapArmed = true;
            }
            state.lastPose = pose;

            switch (pose)
            {
                case Pose::OneFinger: handleOneFinger(hand, state); break;
                case Pose::TwoFinger: handleTwoFinger(hand, state); break;
                case Pose::Pinch:     handlePinch(hand, state);     break;
                case Pose::Fist:      handleFist(hand, state);      break;
                case Pose::None:                                    break;
            }
        }

        // Only refresh prev-tips when we have a committed pose we trust.
        // Refreshing during None would let a "gap frame" poison the velocity
        // calculation on the NEXT real frame, firing spurious taps.
        if (state.lastPose != Pose::None)
            updatePrevTips(hand, state);

        // ── Debug heartbeat (every 15 frames ≈ 8 Hz at 120 fps) ─────────
        if (Config::DEBUG_GESTURES && (frameCount_ % 15 == 0))
        {
            Pose currentPose = state.dragging ? Pose::Fist : classify(hand);
            float pv = hand.palmVelocity().length();
            std::cerr
                << "[f" << frameCount_ << "] pose=" << poseName(currentPose)
                << " ext=" << hand.extendedFingerCount()
                << " [T" << (hand.isExtended(0)?"1":"0")
                <<  "I" << (hand.isExtended(1)?"1":"0")
                <<  "M" << (hand.isExtended(2)?"1":"0")
                <<  "R" << (hand.isExtended(3)?"1":"0")
                <<  "P" << (hand.isExtended(4)?"1":"0") << "]"
                << " grab=" << hand.grab()
                << " pinch=" << hand.pinch()
                << " pinchD=" << hand.pinchDistance()
                << " palmV=" << pv
                << " dragging=" << (state.dragging?"Y":"N")
                << "\n";
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────
// Pose classifier
//
// Priority order matters. A closed fist wins over everything else (grab is
// our drag signal). A "pinch pose" — thumb + index extended, middle curled —
// is distinguished from the two-finger pose (index + middle extended,
// thumb curled) by checking *which* digits are extended, not just the count.
// ─────────────────────────────────────────────────────────────────────────

Pose GestureDetector::classify(const Hand& hand) const
{
    if (hand.grab() > Config::FIST_GRAB_MIN) return Pose::Fist;

    // The thumb's is_extended flag is notoriously noisy — it flickers every
    // few frames even when the user is holding a stable pose. So we classify
    // non-thumb fingers first and only use the thumb to DISAMBIGUATE a pinch
    // (which requires actual thumb-index proximity, not just a flag).
    const bool indexExt  = hand.isExtended(1);
    const bool middleExt = hand.isExtended(2);
    const bool ringExt   = hand.isExtended(3);
    const bool pinkyExt  = hand.isExtended(4);

    // Pinch = thumb & index fingertips close together, other fingers curled.
    // We check actual distance instead of trusting thumb.is_extended.
    const bool pinchGeometry =
        hand.pinchDistance() < Config::PINCH_POSE_MAX_DIST &&
        !middleExt && !ringExt && !pinkyExt;

    if (pinchGeometry) return Pose::Pinch;

    // Index + middle only → two-finger gestures.
    if (indexExt && middleExt && !ringExt && !pinkyExt)
        return Pose::TwoFinger;

    // Index only → cursor. Thumb state ignored.
    if (indexExt && !middleExt && !ringExt && !pinkyExt)
        return Pose::OneFinger;

    return Pose::None;
}

// ─────────────────────────────────────────────────────────────────────────
// Coordinate mapping. Leap gives mm centered on the device; we center on
// the screen and flip Y (Leap Y points up, screen Y points down).
// ─────────────────────────────────────────────────────────────────────────

// In Leap Desktop mode:
//   X = left/right across the device (our screen X)
//   Y = height above the device (up from the desk surface — NOT a horizontal
//       axis; we ignore it for cursor mapping)
//   Z = forward/back across the device, with +Z toward the user and
//       -Z away from the user (our screen Y, inverted: moving your hand
//       AWAY from your body should move the cursor UP on the screen)
CGPoint GestureDetector::tipToScreen(const Vector3& p) const
{
    float sx = ( p.x * Config::CURSOR_SCALE_X) + screenW_ * 0.5f;
    float sy = ( p.z * Config::CURSOR_SCALE_Y) + screenH_ * 0.5f;
    sx = std::clamp(sx, 0.0f, screenW_ - 1);
    sy = std::clamp(sy, 0.0f, screenH_ - 1);
    return CGPointMake(sx, sy);
}

CGPoint GestureDetector::palmToScreen(const Vector3& p) const
{
    return tipToScreen(p);   // same mapping; separate helper for clarity
}

float GestureDetector::indexMiddleAngle(const Hand& hand) const
{
    Vector3 v = hand.fingerTip(2) - hand.fingerTip(1);
    return v.xzAngle();   // horizontal plane in Desktop mode
}

// ─────────────────────────────────────────────────────────────────────────
// One-finger pose: cursor + tap-to-click (left click)
// ─────────────────────────────────────────────────────────────────────────

void GestureDetector::handleOneFinger(const Hand& hand, HandState& state)
{
    moveCursorOneFinger(hand, state);
    detectOneFingerTap(hand, state);
}

void GestureDetector::moveCursorOneFinger(const Hand& hand, HandState& state)
{
    CGPoint raw = tipToScreen(hand.fingerTip(1));

    if (!state.hasPrevPos)
    {
        state.smoothedPos = raw;
        state.hasPrevPos  = true;
        EventInjector::moveCursor(raw);
        return;
    }

    float dx   = raw.x - state.smoothedPos.x;
    float dy   = raw.y - state.smoothedPos.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    // Reject tracking spikes (>180 px/frame is implausible)
    if (dist > 180.0f) { EventInjector::moveCursor(state.smoothedPos); return; }
    // Dead zone — ignore tremor
    if (dist < 3.0f)   { EventInjector::moveCursor(state.smoothedPos); return; }

    // Adaptive smoothing: slow motion → heavy smooth, fast motion → responsive
    float alpha = 0.25f + std::min(dist / 70.0f, 0.60f);
    state.smoothedPos.x = state.smoothedPos.x * (1.0f - alpha) + raw.x * alpha;
    state.smoothedPos.y = state.smoothedPos.y * (1.0f - alpha) + raw.y * alpha;

    EventInjector::moveCursor(state.smoothedPos);
}

// A tap = the index fingertip moves quickly DOWN toward the device (negative
// Y velocity in Desktop mode — Y is height above the device) while the palm
// stays roughly still. We require the tap to be "armed": after firing, the
// fingertip has to come back to near-zero Y velocity before another tap can
// register. That stops one forward jab from firing a burst of clicks.
bool GestureDetector::detectOneFingerTap(const Hand& hand, HandState& state)
{
    if (state.tapCooldown > 0) return false;
    if (!state.hasPrevTips)    return false;

    // Derive index fingertip Y velocity (negative = down toward device)
    // Leap runs ~120 Hz so multiplying by 120 gives mm/s.
    float vy = (hand.fingerTip(1).y - state.prevIndexTip.y) * 120.0f;

    // Palm must be roughly stationary (otherwise this is a swipe, not a tap)
    if (hand.palmVelocity().length() > Config::TAP_PALM_MAX_SPEED) return false;

    if (state.oneFingerTapArmed && vy < -Config::TAP_Z_VELOCITY)
    {
        EventInjector::leftClick(state.smoothedPos);
        state.tapCooldown       = Config::TAP_COOLDOWN_FRAMES;
        state.oneFingerTapArmed = false;
        if (Config::DEBUG_GESTURES) std::cerr << "[fire] LEFT CLICK (vy=" << vy << ")\n";
        return true;
    }

    // Re-arm when the fingertip has pulled back up (positive Y velocity)
    if (!state.oneFingerTapArmed && vy > 50.0f)
        state.oneFingerTapArmed = true;

    return false;
}

// ─────────────────────────────────────────────────────────────────────────
// Two-finger pose: cursor-by-centroid + right-click tap + scroll + swipe
//                   + smart-zoom double tap
//
// Ordering inside a single frame: we try tap first (discrete events take
// priority), then swipe (high velocity), then scroll (lower velocity).
// Only ONE can fire per frame per hand.
// ─────────────────────────────────────────────────────────────────────────

void GestureDetector::handleTwoFinger(const Hand& hand, HandState& state)
{
    moveCursorTwoFinger(hand, state);

    if (detectTwoFingerTap(hand, state)) return;
    if (detectSwipe       (hand, state)) return;
    if (detectScroll      (hand, state)) return;
}

void GestureDetector::moveCursorTwoFinger(const Hand& hand, HandState& state)
{
    Vector3 mid = (hand.fingerTip(1) + hand.fingerTip(2)) * 0.5f;
    CGPoint raw = tipToScreen(mid);

    if (!state.hasPrevPos)
    {
        state.smoothedPos = raw;
        state.hasPrevPos  = true;
        EventInjector::moveCursor(raw);
        return;
    }

    float dx   = raw.x - state.smoothedPos.x;
    float dy   = raw.y - state.smoothedPos.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    if (dist > 180.0f) { EventInjector::moveCursor(state.smoothedPos); return; }
    if (dist < 3.0f)   { EventInjector::moveCursor(state.smoothedPos); return; }

    float alpha = 0.25f + std::min(dist / 70.0f, 0.60f);
    state.smoothedPos.x = state.smoothedPos.x * (1.0f - alpha) + raw.x * alpha;
    state.smoothedPos.y = state.smoothedPos.y * (1.0f - alpha) + raw.y * alpha;

    EventInjector::moveCursor(state.smoothedPos);
}

// Two-finger tap = both index AND middle fingertips jab downward toward the
// device simultaneously. This is how macOS distinguishes a two-finger tap
// from a scroll: the fingertips move down, not the whole palm.
bool GestureDetector::detectTwoFingerTap(const Hand& hand, HandState& state)
{
    if (state.tapCooldown > 0) return false;
    if (!state.hasPrevTips)    return false;
    if (hand.palmVelocity().length() > Config::TAP_PALM_MAX_SPEED) return false;

    float vyIndex  = (hand.fingerTip(1).y - state.prevIndexTip.y ) * 120.0f;
    float vyMiddle = (hand.fingerTip(2).y - state.prevMiddleTip.y) * 120.0f;

    // Both fingertips must stab downward together
    bool bothStabbing =
        vyIndex  < -Config::TAP_Z_VELOCITY &&
        vyMiddle < -Config::TAP_Z_VELOCITY;

    if (state.twoFingerTapArmed && bothStabbing)
    {
        // Check for smart-zoom (second tap within window)
        int gap = frameCount_ - state.lastTwoFingerTapFrame;
        if (gap <= Config::SMART_ZOOM_MAX_GAP_FRAMES &&
            state.smartZoomCooldown == 0)
        {
            EventInjector::smartZoom(state.smoothedPos);
            state.smartZoomCooldown     = 45;
            state.lastTwoFingerTapFrame = -10000;   // consume both taps
            if (Config::DEBUG_GESTURES) std::cerr << "[fire] SMART ZOOM\n";
        }
        else
        {
            EventInjector::rightClick(state.smoothedPos);
            state.lastTwoFingerTapFrame = frameCount_;
            if (Config::DEBUG_GESTURES) std::cerr << "[fire] RIGHT CLICK\n";
        }

        state.tapCooldown       = Config::TAP_COOLDOWN_FRAMES;
        state.twoFingerTapArmed = false;
        return true;
    }

    if (!state.twoFingerTapArmed && vyIndex > 50.0f && vyMiddle > 50.0f)
        state.twoFingerTapArmed = true;

    return false;
}

bool GestureDetector::detectSwipe(const Hand& hand, HandState& state)
{
    if (state.swipeCooldown > 0) return false;

    float vx = hand.palmVelocity().x;

    // Count consecutive frames the velocity has exceeded the threshold in
    // each direction. A single-frame spike does nothing; you have to
    // actually sweep your hand for a few frames to fire.
    if (vx > Config::SWIPE_VELOCITY_THRESHOLD) {
        ++state.swipeRightFrames;
        state.swipeLeftFrames = 0;
    } else if (vx < -Config::SWIPE_VELOCITY_THRESHOLD) {
        ++state.swipeLeftFrames;
        state.swipeRightFrames = 0;
    } else {
        state.swipeRightFrames = 0;
        state.swipeLeftFrames  = 0;
    }

    if (state.swipeRightFrames >= Config::SWIPE_SUSTAIN_FRAMES) {
        EventInjector::swipeRight();
        state.swipeCooldown    = Config::SWIPE_COOLDOWN_FRAMES;
        state.swipeRightFrames = 0;
        if (Config::DEBUG_GESTURES) std::cerr << "[fire] swipe RIGHT (vx=" << vx << ")\n";
        return true;
    }
    if (state.swipeLeftFrames >= Config::SWIPE_SUSTAIN_FRAMES) {
        EventInjector::swipeLeft();
        state.swipeCooldown   = Config::SWIPE_COOLDOWN_FRAMES;
        state.swipeLeftFrames = 0;
        if (Config::DEBUG_GESTURES) std::cerr << "[fire] swipe LEFT (vx=" << vx << ")\n";
        return true;
    }
    return false;
}

bool GestureDetector::detectScroll(const Hand& hand, HandState& state)
{
    // In Desktop mode, scroll direction is driven by horizontal palm motion:
    //   Z = forward/back (toward/away from user) → vertical scroll
    //   X = left/right                           → horizontal scroll
    // Palm Y would be hand-height, which has no business driving scroll.
    float vScrollY = hand.palmVelocity().z;   // forward/back → scroll up/down
    float vScrollX = hand.palmVelocity().x;

    if (std::abs(vScrollY) < Config::SCROLL_MIN_SPEED &&
        std::abs(vScrollX) < Config::SCROLL_MIN_SPEED)
        return false;

    EventInjector::scroll(
        static_cast<int32_t>( vScrollY * Config::SCROLL_SENSITIVITY),
        static_cast<int32_t>(-vScrollX * Config::SCROLL_SENSITIVITY)
    );
    if (Config::DEBUG_GESTURES && (frameCount_ % 10 == 0))
        std::cerr << "[fire] SCROLL vY=" << vScrollY << " vX=" << vScrollX << "\n";
    return true;
}

// ─────────────────────────────────────────────────────────────────────────
// Pinch pose: zoom (thumb↔index distance change) + rotate (thumb↔index
// vector angle change). Same hand pose, different signals — they can
// fire in the same frame because they're measuring orthogonal things.
// ─────────────────────────────────────────────────────────────────────────

void GestureDetector::handlePinch(const Hand& hand, HandState& state)
{
    detectZoom  (hand, state);
    detectRotate(hand, state);
}

bool GestureDetector::detectZoom(const Hand& hand, HandState& state)
{
    // Use the SDK's own pinch_distance — it's already smoothed and
    // calibrated for the actual thumb/index tips.
    float dist = hand.pinchDistance();

    if (state.prevPinchDistance < 0.0f)
    {
        state.prevPinchDistance = dist;
        return false;
    }

    float delta = dist - state.prevPinchDistance;
    state.prevPinchDistance = dist;

    if (std::abs(delta) < Config::PINCH_NOISE_FLOOR) return false;

    EventInjector::zoom(delta * Config::PINCH_SCALE, 1);
    if (Config::DEBUG_GESTURES && (frameCount_ % 10 == 0))
        std::cerr << "[fire] ZOOM delta=" << delta << "\n";
    return true;
}

bool GestureDetector::detectRotate(const Hand& hand, HandState& state)
{
    // Rotation: angle of thumb→index vector in the horizontal (XZ) plane.
    // In Desktop mode the fingertips sweep the XZ plane as the wrist twists.
    Vector3 v = hand.fingerTip(1) - hand.fingerTip(0);
    float angle = v.xzAngle();

    if (!state.hasPrevAngle)
    {
        state.prevIndexMiddleAngle = angle;
        state.hasPrevAngle         = true;
        return false;
    }

    float delta = angle - state.prevIndexMiddleAngle;
    // Wrap-around guard (atan2 jumps at ±π)
    if (delta >  M_PI) delta -= 2.0f * M_PI;
    if (delta < -M_PI) delta += 2.0f * M_PI;
    state.prevIndexMiddleAngle = angle;

    if (std::abs(delta) < Config::ROTATE_MIN_DELTA) return false;

    EventInjector::rotate(delta * Config::ROTATE_SCALE, 1);
    if (Config::DEBUG_GESTURES && (frameCount_ % 10 == 0))
        std::cerr << "[fire] ROTATE delta=" << delta << "\n";
    return true;
}

// ─────────────────────────────────────────────────────────────────────────
// Fist pose: drag & drop.
//
// State machine, gated by hysteresis + stability counter:
//   - Not dragging + grab > ON for N frames   →  mouseDown, dragging = true
//   - Dragging     + grab < OFF for N frames  →  mouseUp,   dragging = false
//   - Dragging     + palm moves               →  mouseDragged at palm position
//
// Using the palm (not fingertip) while grabbing makes physical sense — when
// you close your fist your fingertips are hidden anyway, and the palm is
// the natural "held" point.
// ─────────────────────────────────────────────────────────────────────────

void GestureDetector::handleFist(const Hand& hand, HandState& state)
{
    CGPoint palmScreen = palmToScreen(hand.palmPosition());

    if (!state.dragging)
    {
        // Looking for a stable rising edge on grab
        if (hand.grab() > Config::GRAB_ON_THRESHOLD)
        {
            ++state.grabRisingFrames;
            state.grabFallingFrames = 0;
            if (state.grabRisingFrames >= Config::GRAB_STABILITY_FRAMES)
            {
                state.dragging   = true;
                state.dragAnchor = palmScreen;
                EventInjector::mouseDown(palmScreen);
                if (Config::DEBUG_GESTURES)
                    std::cerr << "[fire] DRAG START at ("
                              << palmScreen.x << "," << palmScreen.y << ")\n";
            }
        }
        else
        {
            state.grabRisingFrames = 0;
        }
    }
    else
    {
        // Dragging: look for stable falling edge, else emit drag moves
        if (hand.grab() < Config::GRAB_OFF_THRESHOLD)
        {
            ++state.grabFallingFrames;
            state.grabRisingFrames = 0;
            if (state.grabFallingFrames >= Config::GRAB_STABILITY_FRAMES)
            {
                EventInjector::mouseUp(palmScreen);
                state.dragging          = false;
                state.grabFallingFrames = 0;
                if (Config::DEBUG_GESTURES)
                    std::cerr << "[fire] DROP at ("
                              << palmScreen.x << "," << palmScreen.y << ")\n";
            }
            else
            {
                // Still considered "holding" during the release window
                EventInjector::mouseDragged(palmScreen);
            }
        }
        else
        {
            state.grabFallingFrames = 0;
            EventInjector::mouseDragged(palmScreen);
        }
        state.smoothedPos = palmScreen;  // keep cursor pos in sync
    }
}

// ─────────────────────────────────────────────────────────────────────────
// Bookkeeping
// ─────────────────────────────────────────────────────────────────────────

void GestureDetector::updatePrevTips(const Hand& hand, HandState& state)
{
    state.prevThumbTip  = hand.fingerTip(0);
    state.prevIndexTip  = hand.fingerTip(1);
    state.prevMiddleTip = hand.fingerTip(2);
    state.hasPrevTips   = true;
}

void GestureDetector::resetTwoFingerState(HandState& state)
{
    state.twoFingerTapArmed = true;
    // lastTwoFingerTapFrame intentionally preserved — a user can briefly
    // leave the two-finger pose between the two taps of a smart-zoom.
}

void GestureDetector::resetPinchState(HandState& state)
{
    state.prevPinchDistance    = -1.0f;
    state.hasPrevAngle         = false;
    state.prevIndexMiddleAngle = 0.0f;
}