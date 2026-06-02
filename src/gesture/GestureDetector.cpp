#include "GestureDetector.h"
#include "../output/EventInjector.h"
#include "../core/Config.h"
#include <cmath>
#include <algorithm>
#include <iostream>

// Helper: return the short name for a Pose (for debug logging)
static const char *poseName(Pose p) {
    switch (p) {
        case Pose::None: return "None";
        case Pose::Fist: return "Fist";
        case Pose::OneFinger: return "OneFinger";
        case Pose::TwoFinger: return "TwoFinger";
        case Pose::Pinch: return "Pinch";
    }
    return "?";
}

// ─────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────

GestureDetector::GestureDetector() {
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

void GestureDetector::update(const Frame &frame) {
    ++frameCount_;

    for (const auto &hand: frame.hands()) {
        HandState &state = states_[hand.id()];

        // Decrement cooldowns once per frame
        if (state.tapCooldown > 0) --state.tapCooldown;
        if (state.swipeCooldown > 0) --state.swipeCooldown;
        if (state.smartZoomCooldown > 0) --state.smartZoomCooldown;
        if (state.postDragScrollCooldown > 0) --state.postDragScrollCooldown;

        // The drag gesture is pose-independent: a grab that starts as a
        // fist must continue dragging even if the user transiently opens
        // a finger. So we update drag state FIRST, based on grab_strength
        // alone, and only if we're not dragging do we classify a fresh pose.
        if (state.dragging) {
            handleFist(hand, state);
        } else {
            // ── Pose hysteresis ─────────────────────────────────────
            // classify() looks at the current frame only. But Leap's flags
            // flicker, so we apply a stability window: a new raw pose must
            // appear for N consecutive frames before it becomes the "committed"
            // pose that handlers actually see. This kills cursor jumps and
            // spurious clicks caused by 1-2 frame thumb flickers.
            Pose raw = classify(hand);
            if (raw == state.candidatePose) {
                if (state.candidateFrames < Config::POSE_STABILITY_FRAMES)
                    ++state.candidateFrames;
            } else {
                state.candidatePose = raw;
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
            if (pose != state.lastPose) {
                if (Config::DEBUG_GESTURES)
                    std::cerr << "[pose] " << poseName(state.lastPose)
                            << " -> " << poseName(pose) << "\n";

                if (state.lastPose == Pose::TwoFinger) resetTwoFingerState(state, frameCount_);
                if (state.lastPose == Pose::Pinch) {
                    state.lastPinchFrame = frameCount_;
                    resetPinchState(state);
                }
                // Drop stale cursor/tip history — it's from a different pose
                // whose coordinates don't correspond to the new one's.
                state.hasPrevPos = false;
                state.hasPrevTips = false;
                if (pose == Pose::OneFinger) state.oneFingerTapArmed = true;
                if (pose == Pose::TwoFinger) state.twoFingerTapArmed = true;
                if (pose == Pose::Pinch) state.tapCooldown = 20;

                // CRITICAL: when leaving Pinch, the index fingertip is moving
                // fast (releasing the pinch). That motion has high vy and
                // can falsely trigger a tap. Lock out tap detection for a
                // generous window after any Pinch->X transition so the
                // release motion doesn't click random things in the app.
                if (state.lastPose == Pose::Pinch) {
                    state.tapCooldown = 30; // ~250 ms at 120 fps
                    state.oneFingerTapArmed = false; // require re-arm
                    state.twoFingerTapArmed = false;
                }
            }
            state.lastPose = pose;

            switch (pose) {
                case Pose::OneFinger: handleOneFinger(hand, state);
                    break;
                case Pose::TwoFinger: handleTwoFinger(hand, state);
                    break;
                case Pose::Pinch: handlePinch(hand, state);
                    break;
                case Pose::Fist: handleFist(hand, state);
                    break;
                case Pose::None: break;
            }
        }

        // Only refresh prev-tips when we have a committed pose we trust.
        // Refreshing during None would let a "gap frame" poison the velocity
        // calculation on the NEXT real frame, firing spurious taps.
        if (state.lastPose != Pose::None)
            updatePrevTips(hand, state);

        // ── Debug heartbeat (every 15 frames ≈ 8 Hz at 120 fps) ─────────
        if (Config::DEBUG_GESTURES && (frameCount_ % 15 == 0)) {
            Pose currentPose = state.dragging ? Pose::Fist : classify(hand);
            float pv = hand.palmVelocity().length();
            std::cerr
                    << "[f" << frameCount_ << "] pose=" << poseName(currentPose)
                    << " ext=" << hand.extendedFingerCount()
                    << " [T" << (hand.isExtended(0) ? "1" : "0")
                    << "I" << (hand.isExtended(1) ? "1" : "0")
                    << "M" << (hand.isExtended(2) ? "1" : "0")
                    << "R" << (hand.isExtended(3) ? "1" : "0")
                    << "P" << (hand.isExtended(4) ? "1" : "0") << "]"
                    << " grab=" << hand.grab()
                    << " pinch=" << hand.pinch()
                    << " pinchD=" << hand.pinchDistance()
                    << " palmV=" << pv
                    << " dragging=" << (state.dragging ? "Y" : "N")
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

Pose GestureDetector::classify(const Hand &hand) const {
    // Fist requires a strong grab WITH a low pinch. A tight pinch gesture
    // also produces high grab_strength (because thumb+index coming together
    // looks like half a fist), so we reject those by also requiring the
    // pinch signal to be low — in a real fist the thumb is curled in, not
    // pinching the index.
    if (hand.grab() > Config::FIST_GRAB_MIN &&
        hand.pinch() < Config::FIST_MAX_PINCH)
        return Pose::Fist;

    // The thumb's is_extended flag is notoriously noisy — it flickers every
    // few frames even when the user is holding a stable pose. So we classify
    // non-thumb fingers first and only use the thumb to DISAMBIGUATE a pinch
    // (which requires actual thumb-index proximity, not just a flag).
    const bool indexExt = hand.isExtended(1);
    const bool middleExt = hand.isExtended(2);
    const bool ringExt = hand.isExtended(3);
    const bool pinkyExt = hand.isExtended(4);

    // Pinch pose is ONLY when the user is actively pinching. The bar is
    // intentionally very high (0.95) because pinch_strength transiently
    // spikes to 0.7–0.8 during fast two-finger motion even when the user
    // isn't pinching. Only a sustained true pinch crosses this threshold.
    const bool pinchActive =
            hand.pinch() > Config::PINCH_POSE_MIN_STRENGTH &&
            hand.pinchDistance() < Config::PINCH_POSE_MAX_DIST &&
            !middleExt && !ringExt && !pinkyExt;

    if (pinchActive) return Pose::Pinch;

    // Index + middle only → two-finger gestures.
    // Note: this is a STRICT pose — must be the index+middle configuration
    // (the macOS trackpad two-finger gesture). Holding thumb + index is
    // OneFinger, not TwoFinger, because we can't reliably tell intentional
    // "two fingers" from a relaxed hand with thumb out.
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
CGPoint GestureDetector::tipToScreen(const Vector3 &p) const {
    float sx = (p.x - Config::LEAP_X_MIN) / (Config::LEAP_X_MAX - Config::LEAP_X_MIN) * screenW_;
    float sy = (p.z - Config::LEAP_Z_MIN) / (Config::LEAP_Z_MAX - Config::LEAP_Z_MIN) * screenH_;
    sx = std::clamp(sx, 0.0f, screenW_ - 1);
    sy = std::clamp(sy, 0.0f, screenH_ - 1);
    return CGPointMake(sx, sy);
}

CGPoint GestureDetector::palmToScreen(const Vector3 &p) const {
    return tipToScreen(p);
}

float GestureDetector::indexMiddleAngle(const Hand &hand) const {
    Vector3 v = hand.fingerTip(2) - hand.fingerTip(1);
    return v.xzAngle(); // horizontal plane in Desktop mode
}

// ─────────────────────────────────────────────────────────────────────────
// One-finger pose: cursor + tap-to-click (left click)
// ─────────────────────────────────────────────────────────────────────────

void GestureDetector::handleOneFinger(const Hand &hand, HandState &state) {
    moveCursorOneFinger(hand, state);
    detectOneFingerTap(hand, state);
}

void GestureDetector::moveCursorOneFinger(const Hand &hand, HandState &state) {
    CGPoint raw = tipToScreen(hand.fingerTip(1));

    if (!state.hasPrevPos) {
        state.smoothedPos = raw;
        state.hasPrevPos = true;
        EventInjector::moveCursor(raw);
        return;
    }

    float dx = raw.x - state.smoothedPos.x;
    float dy = raw.y - state.smoothedPos.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    // Reject tracking spikes (>180 px/frame is implausible)
    if (dist > 180.0f) {
        EventInjector::moveCursor(state.smoothedPos);
        return;
    }

    // Dead zone — at scale 5.0 hand tremor produces ~5px wobble.
    // 6 px dead zone keeps cursor still during careful aiming. We still
    // update stablePos here, because if the user is holding still over a
    // target and then taps, the click should fire at the stable smoothed
    // position (THE TARGET), not at a stale value from before the pause.
    if (dist < 4.0f) {
        state.stablePos = state.smoothedPos; // refresh every still frame
        state.hasStablePos = true;
        EventInjector::moveCursor(state.smoothedPos);
        return;
    }

    // Adaptive smoothing: slow motion → heavy smooth, fast motion → responsive
    float alpha = 0.25f + std::min(dist / 70.0f, 0.60f);
    state.smoothedPos.x = state.smoothedPos.x * (1.0f - alpha) + raw.x * alpha;
    state.smoothedPos.y = state.smoothedPos.y * (1.0f - alpha) + raw.y * alpha;

    // Track the stable hover position for clicks. Update whenever the user
    // is holding still or moving slowly — any motion below this threshold
    // is "aiming," anything above is "jabbing for a tap."
    bool palmStill = hand.palmVelocity().length() < 40.0f;
    bool fingerStill = state.hasPrevTips
                       && std::abs(hand.fingerTip(1).y - state.prevIndexTip.y) * 120.0f < 80.0f;
    if (palmStill && fingerStill) {
        state.stablePos = state.smoothedPos;
        state.hasStablePos = true;
    }

    EventInjector::moveCursor(state.smoothedPos);
}

// A tap = the index fingertip moves quickly DOWN toward the device (negative
// Y velocity in Desktop mode — Y is height above the device) while the palm
// stays roughly still. We require the tap to be "armed": after firing, the
// fingertip has to come back to near-zero Y velocity before another tap can
// register. That stops one forward jab from firing a burst of clicks.
bool GestureDetector::detectOneFingerTap(const Hand &hand, HandState &state) {
    if (state.tapCooldown > 0) return false;
    if (!state.hasPrevTips) return false;

    // Derive index fingertip Y velocity (negative = down toward device)
    // Leap runs ~120 Hz so multiplying by 120 gives mm/s.
    float vy = (hand.fingerTip(1).y - state.prevIndexTip.y) * 120.0f;

    // Palm must be roughly stationary (otherwise this is a swipe, not a tap)
    if (hand.palmVelocity().length() > Config::TAP_PALM_MAX_SPEED) return false;

    if (state.oneFingerTapArmed && vy < -Config::TAP_Z_VELOCITY) {
        // Click at the stable hover position (where the user was aiming),
        // not the current smoothed position (which drifted during the jab).
        // We also warp the cursor there first so the click event lands on
        // the right pixel — macOS click events use the cursor location at
        // the moment the event is processed.
        CGPoint clickPos = state.hasStablePos ? state.stablePos : state.smoothedPos;
        EventInjector::moveCursor(clickPos);
        EventInjector::leftClick(clickPos);
        state.smoothedPos = clickPos; // resync smoothed pos to the click point
        state.tapCooldown = Config::TAP_COOLDOWN_FRAMES;
        state.oneFingerTapArmed = false;
        if (Config::DEBUG_GESTURES)
            std::cerr << "[fire] LEFT CLICK at (" << clickPos.x << "," << clickPos.y
                    << ") vy=" << vy << "\n";
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

void GestureDetector::handleTwoFinger(const Hand &hand, HandState &state) {
    moveCursorTwoFinger(hand, state);

    if (detectTwoFingerTap(hand, state)) return;
    if (detectSwipe(hand, state)) return;
    if (detectScroll(hand, state)) return;
}

void GestureDetector::moveCursorTwoFinger(const Hand &hand, HandState &state) {
    Vector3 mid = (hand.fingerTip(1) + hand.fingerTip(2)) * 0.5f;
    CGPoint raw = tipToScreen(mid);

    if (!state.hasPrevPos) {
        state.smoothedPos = raw;
        state.hasPrevPos = true;
        EventInjector::moveCursor(raw);
        return;
    }

    float dx = raw.x - state.smoothedPos.x;
    float dy = raw.y - state.smoothedPos.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    if (dist > 180.0f) {
        EventInjector::moveCursor(state.smoothedPos);
        return;
    }
    if (dist < 3.0f) {
        EventInjector::moveCursor(state.smoothedPos);
        return;
    }

    float alpha = 0.25f + std::min(dist / 70.0f, 0.60f);
    state.smoothedPos.x = state.smoothedPos.x * (1.0f - alpha) + raw.x * alpha;
    state.smoothedPos.y = state.smoothedPos.y * (1.0f - alpha) + raw.y * alpha;

    bool palmStill = hand.palmVelocity().length() < 60.0f;
    if (palmStill) {
        state.stablePos = state.smoothedPos;
        state.hasStablePos = true;
    }

    EventInjector::moveCursor(state.smoothedPos);
}

// Two-finger tap = both index AND middle fingertips jab downward toward the
// device simultaneously. This is how macOS distinguishes a two-finger tap
// from a scroll: the fingertips move down, not the whole palm.
bool GestureDetector::detectTwoFingerTap(const Hand &hand, HandState &state) {
    if (state.tapCooldown > 0) return false;
    if (!state.hasPrevTips) return false;
    if (hand.palmVelocity().length() > Config::TAP_PALM_MAX_SPEED) return false;

    float vyIndex = (hand.fingerTip(1).y - state.prevIndexTip.y) * 120.0f;
    float vyMiddle = (hand.fingerTip(2).y - state.prevMiddleTip.y) * 120.0f;

    // Both fingertips must stab downward together
    bool bothStabbing =
            vyIndex < -Config::TAP_Z_VELOCITY &&
            vyMiddle < -Config::TAP_Z_VELOCITY;

    if (state.twoFingerTapArmed && bothStabbing) {
        CGPoint clickPos = state.hasStablePos ? state.stablePos : state.smoothedPos;

        // Check for smart-zoom (second tap within window)
        int gap = frameCount_ - state.lastTwoFingerTapFrame;
        if (gap <= Config::SMART_ZOOM_MAX_GAP_FRAMES &&
            state.smartZoomCooldown == 0) {
            EventInjector::moveCursor(clickPos);
            EventInjector::smartZoom(clickPos);
            state.smartZoomCooldown = 45;
            state.lastTwoFingerTapFrame = -10000;
            if (Config::DEBUG_GESTURES) std::cerr << "[fire] SMART ZOOM\n";
        } else {
            EventInjector::moveCursor(clickPos);
            EventInjector::rightClick(clickPos);
            state.lastTwoFingerTapFrame = frameCount_;
            if (Config::DEBUG_GESTURES) std::cerr << "[fire] RIGHT CLICK\n";
        }

        state.smoothedPos = clickPos;
        state.tapCooldown = Config::TAP_COOLDOWN_FRAMES;
        state.twoFingerTapArmed = false;
        return true;
    }

    if (!state.twoFingerTapArmed && vyIndex > 50.0f && vyMiddle > 50.0f)
        state.twoFingerTapArmed = true;

    return false;
}

bool GestureDetector::detectSwipe(const Hand &hand, HandState &state) {
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
        state.swipeLeftFrames = 0;
    }

    if (state.swipeRightFrames >= Config::SWIPE_SUSTAIN_FRAMES) {
        EventInjector::swipeRight();
        state.swipeCooldown = Config::SWIPE_COOLDOWN_FRAMES;
        state.swipeRightFrames = 0;
        if (Config::DEBUG_GESTURES) std::cerr << "[fire] swipe RIGHT (vx=" << vx << ")\n";
        return true;
    }
    if (state.swipeLeftFrames >= Config::SWIPE_SUSTAIN_FRAMES) {
        EventInjector::swipeLeft();
        state.swipeCooldown = Config::SWIPE_COOLDOWN_FRAMES;
        state.swipeLeftFrames = 0;
        if (Config::DEBUG_GESTURES) std::cerr << "[fire] swipe LEFT (vx=" << vx << ")\n";
        return true;
    }
    return false;
}

bool GestureDetector::detectScroll(const Hand &hand, HandState &state) {
    if (state.swipeCooldown > 0) return false;
    if (state.postDragScrollCooldown > 0) return false;
    if (frameCount_ - state.lastScrollFrame < 4) return false;

    float vScrollY = hand.palmVelocity().z;
    if (std::abs(vScrollY) < Config::SCROLL_MIN_SPEED) return false;

    float clamped = std::clamp(vScrollY, -400.0f, 400.0f);
    state.lastScrollFrame = frameCount_;

    EventInjector::scroll(
        static_cast<int32_t>(clamped * Config::SCROLL_SENSITIVITY),
        0
    );
    if (Config::DEBUG_GESTURES)
        std::cerr << "[fire] SCROLL vY=" << vScrollY
                << " clamped=" << clamped << "\n";
    return true;
}

// ─────────────────────────────────────────────────────────────────────────
// Pinch pose: zoom (thumb↔index distance change) + rotate (thumb↔index
// vector angle change). Same hand pose, different signals — they can
// fire in the same frame because they're measuring orthogonal things.
// ─────────────────────────────────────────────────────────────────────────

void GestureDetector::handlePinch(const Hand& hand, HandState& state)
{
    // Freeze cursor during pinch — don't move it while zooming
    EventInjector::moveCursor(state.smoothedPos);

    detectZoom  (hand, state);
    detectRotate(hand, state);
}

bool GestureDetector::detectZoom(const Hand& hand, HandState& state)
{
    float dist = hand.pinchDistance();

    if (state.prevPinchDistance < 0.0f) {
        state.prevPinchDistance = dist;
        return false;
    }

    float delta = dist - state.prevPinchDistance;
    state.prevPinchDistance = dist;

    if (std::abs(delta) < Config::PINCH_NOISE_FLOOR) return false;

    // Throttle — one zoom step per 8 frames (~67ms at 120Hz)
    if (frameCount_ - state.lastZoomFrame < 8) return false;

    state.lastZoomFrame = frameCount_;

    if (state.hasStablePos)
        EventInjector::moveCursor(state.stablePos);

    EventInjector::zoom(delta * Config::PINCH_SCALE, 1);
    if (Config::DEBUG_GESTURES)
        std::cerr << "[fire] ZOOM delta=" << delta << "\n";
    return true;
}

bool GestureDetector::detectRotate(const Hand &hand, HandState &state) {
    // Rotation: angle of thumb→index vector in the horizontal (XZ) plane.
    // In Desktop mode the fingertips sweep the XZ plane as the wrist twists.
    Vector3 v = hand.fingerTip(1) - hand.fingerTip(0);
    float angle = v.xzAngle();

    if (!state.hasPrevAngle) {
        state.prevIndexMiddleAngle = angle;
        state.hasPrevAngle = true;
        return false;
    }

    float delta = angle - state.prevIndexMiddleAngle;
    // Wrap-around guard (atan2 jumps at ±π)
    if (delta > M_PI) delta -= 2.0f * M_PI;
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

void GestureDetector::handleFist(const Hand &hand, HandState &state) {
    if (!state.dragging) {
        // Looking for a stable rising edge on grab
        if (hand.grab() > Config::GRAB_ON_THRESHOLD) {
            ++state.grabRisingFrames;
            state.grabFallingFrames = 0;
            if (state.grabRisingFrames >= Config::GRAB_STABILITY_FRAMES) {
                state.dragging = true;
                // Drag starts from the cursor's CURRENT on-screen position —
                // where the user was pointing before they closed the fist.
                // Do NOT use palmToScreen here: when the hand is closed, palm
                // coordinates often clamp to screen edges because the palm
                // origin sits outside the tracking volume we mapped for tips.
                state.dragAnchor = state.smoothedPos;
                state.dragPalmStart = hand.palmPosition(); // remember origin for deltas
                state.hasDragPalmStart = true;
                EventInjector::mouseDown(state.dragAnchor);
                if (Config::DEBUG_GESTURES)
                    std::cerr << "[fire] DRAG START at ("
                            << state.dragAnchor.x << "," << state.dragAnchor.y << ")\n";
            }
        } else {
            state.grabRisingFrames = 0;
        }
    } else {
        // Compute current cursor target = anchor + (palm displacement since grab)
        // Scales palm motion the same way fingertips were scaled so motion
        // feels consistent across poses.
        CGPoint target = state.dragAnchor;
        if (state.hasDragPalmStart) {
            float dx = (hand.palmPosition().x - state.dragPalmStart.x)
                       * Config::CURSOR_SCALE_X;
            float dz = (hand.palmPosition().z - state.dragPalmStart.z)
                       * Config::CURSOR_SCALE_Y;
            target.x = std::clamp(state.dragAnchor.x + dx, 0.0, (double) screenW_ - 1);
            target.y = std::clamp(state.dragAnchor.y + dz, 0.0, (double) screenH_ - 1);
        }

        // Dragging: look for stable falling edge, else emit drag moves
        if (hand.grab() < Config::GRAB_OFF_THRESHOLD) {
            ++state.grabFallingFrames;
            state.grabRisingFrames = 0;
            if (state.grabFallingFrames >= Config::GRAB_STABILITY_FRAMES) {
                EventInjector::mouseUp(target);
                state.dragging = false;
                state.grabFallingFrames = 0;
                state.hasDragPalmStart = false;
                state.postDragScrollCooldown = 20;
                if (Config::DEBUG_GESTURES)
                    std::cerr << "[fire] DROP at ("
                            << target.x << "," << target.y << ")\n";
            } else {
                EventInjector::mouseDragged(target);
            }
        } else {
            state.grabFallingFrames = 0;
            EventInjector::mouseDragged(target);
        }
        state.smoothedPos = target; // cursor stays in sync for the next pose
    }
}




// ─────────────────────────────────────────────────────────────────────────
// Bookkeeping
// ─────────────────────────────────────────────────────────────────────────

void GestureDetector::updatePrevTips(const Hand &hand, HandState &state) {
    state.prevThumbTip = hand.fingerTip(0);
    state.prevIndexTip = hand.fingerTip(1);
    state.prevMiddleTip = hand.fingerTip(2);
    state.hasPrevTips = true;
}

void GestureDetector::resetTwoFingerState(HandState &state, int currentFrame) {
    state.twoFingerTapArmed = true;
    state.lastScrollFrame = currentFrame;
}

void GestureDetector::resetPinchState(HandState& state) {
    state.hasPrevAngle         = false;
    state.prevIndexMiddleAngle = 0.0f;
}
