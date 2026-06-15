#include "GestureDetector.h"
#include "../output/EventInjector.h"
#include "../core/Config.h"
#include <cmath>
#include <algorithm>
#include <iostream>

static const char *poseName(Pose p) {
    switch (p) {
        case Pose::None: return "None";
        case Pose::Fist: return "Fist";
        case Pose::OneFinger: return "OneFinger";
        case Pose::TwoFinger: return "TwoFinger";
        case Pose::Pinch: return "Pinch";
        case Pose::OpenHand: return "OpenHand";
    }
    return "?";
}

GestureDetector::GestureDetector() {
    CGDirectDisplayID display = CGMainDisplayID();
    screenW_ = static_cast<float>(CGDisplayPixelsWide(display));
    screenH_ = static_cast<float>(CGDisplayPixelsHigh(display));
}


void GestureDetector::update(const Frame &frame) {
    ++frameCount_;

    for (const auto &hand: frame.hands()) {
        HandState &state = states_[hand.id()];

        if (state.tapCooldown > 0) --state.tapCooldown;
        if (state.swipeCooldown > 0) --state.swipeCooldown;
        if (state.smartZoomCooldown > 0) --state.smartZoomCooldown;
        if (state.postDragScrollCooldown > 0) --state.postDragScrollCooldown;

        if (state.dragging) {
            handleFist(hand, state);
        } else {
            Pose raw = classify(hand);
            if (raw == state.candidatePose) {
                if (state.candidateFrames < Config::POSE_STABILITY_FRAMES)
                    ++state.candidateFrames;
            } else {
                state.candidatePose = raw;
                state.candidateFrames = 1;
            }
            Pose pose = state.lastPose;
            if (state.candidateFrames >= Config::POSE_STABILITY_FRAMES)
                pose = state.candidatePose;
            if (pose != state.lastPose) {
                if (Config::DEBUG_GESTURES)
                    std::cerr << "[pose] " << poseName(state.lastPose)
                            << " -> " << poseName(pose) << "\n";

                if (state.lastPose == Pose::TwoFinger) resetTwoFingerState(state, frameCount_);
                if (state.lastPose == Pose::Pinch) {
                    state.lastPinchFrame = frameCount_;
                    resetPinchState(state);
                }
                state.hasPrevPos = false;
                state.hasPrevTips = false;
                if (pose == Pose::OneFinger) state.oneFingerTapArmed = true;
                if (pose == Pose::TwoFinger) state.twoFingerTapArmed = true;
                if (pose == Pose::Pinch) {
                    state.tapCooldown = 20;
                    state.prevPinchDistance = -1.0f;
                }
                if (pose == Pose::OpenHand) {
                    state.hasPrevAngle = false;
                    state.rotAccumDeg = 0.0f;
                }

                if (state.lastPose == Pose::Pinch) {
                    state.tapCooldown = 30;
                    state.oneFingerTapArmed = false;
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
                case Pose::OpenHand: handleOpenHand(hand, state);
                    break;
                case Pose::Fist: handleFist(hand, state);
                    break;
                case Pose::None: break;
            }
        }

        if (state.lastPose != Pose::None)
            updatePrevTips(hand, state);

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


Pose GestureDetector::classify(const Hand &hand) const {
    if (hand.grab() > Config::FIST_GRAB_MIN &&
        hand.pinch() < Config::FIST_MAX_PINCH)
        return Pose::Fist;
    const bool indexExt = hand.isExtended(1);
    const bool middleExt = hand.isExtended(2);
    const bool ringExt = hand.isExtended(3);
    const bool pinkyExt = hand.isExtended(4);
    const bool pinchActive =
            hand.pinch() > Config::PINCH_POSE_MIN_STRENGTH &&
            hand.pinchDistance() < Config::PINCH_POSE_MAX_DIST &&
            !middleExt && !ringExt && !pinkyExt;

    if (pinchActive) return Pose::Pinch;

    if (indexExt && middleExt && ringExt && pinkyExt)
        return Pose::OpenHand;

    if (indexExt && middleExt && !ringExt && !pinkyExt)
        return Pose::TwoFinger;

    if (indexExt && !middleExt && !ringExt && !pinkyExt)
        return Pose::OneFinger;

    return Pose::None;
}

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
    return v.xzAngle();
}


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

    if (dist > 180.0f) {
        EventInjector::moveCursor(state.smoothedPos);
        return;
    }
    if (dist < 4.0f) {
        state.stablePos = state.smoothedPos;
        state.hasStablePos = true;
        EventInjector::moveCursor(state.smoothedPos);
        return;
    }

    float alpha = 0.25f + std::min(dist / 70.0f, 0.60f);
    state.smoothedPos.x = state.smoothedPos.x * (1.0f - alpha) + raw.x * alpha;
    state.smoothedPos.y = state.smoothedPos.y * (1.0f - alpha) + raw.y * alpha;

    bool palmStill = hand.palmVelocity().length() < 40.0f;
    bool fingerStill = state.hasPrevTips
                       && std::abs(hand.fingerTip(1).y - state.prevIndexTip.y) * 120.0f < 80.0f;
    if (palmStill && fingerStill) {
        state.stablePos = state.smoothedPos;
        state.hasStablePos = true;
    }

    EventInjector::moveCursor(state.smoothedPos);
}

bool GestureDetector::detectOneFingerTap(const Hand &hand, HandState &state) {
    if (state.tapCooldown > 0) return false;
    if (!state.hasPrevTips) return false;

    float vy = (hand.fingerTip(1).y - state.prevIndexTip.y) * 120.0f;

    if (hand.palmVelocity().length() > Config::TAP_PALM_MAX_SPEED) return false;

    if (state.oneFingerTapArmed && vy < -Config::TAP_Z_VELOCITY) {
        CGPoint clickPos = state.hasStablePos ? state.stablePos : state.smoothedPos;
        EventInjector::moveCursor(clickPos);
        EventInjector::leftClick(clickPos);
        state.smoothedPos = clickPos;
        state.tapCooldown = Config::TAP_COOLDOWN_FRAMES;
        state.oneFingerTapArmed = false;
        if (Config::DEBUG_GESTURES)
            std::cerr << "[fire] LEFT CLICK at (" << clickPos.x << "," << clickPos.y
                    << ") vy=" << vy << "\n";
        return true;
    }

    if (!state.oneFingerTapArmed && vy > 50.0f)
        state.oneFingerTapArmed = true;

    return false;
}

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

bool GestureDetector::detectTwoFingerTap(const Hand &hand, HandState &state) {
    if (state.tapCooldown > 0) return false;
    if (!state.hasPrevTips) return false;
    if (hand.palmVelocity().length() > Config::TAP_PALM_MAX_SPEED) return false;

    float vyIndex = (hand.fingerTip(1).y - state.prevIndexTip.y) * 120.0f;
    float vyMiddle = (hand.fingerTip(2).y - state.prevMiddleTip.y) * 120.0f;

    bool bothStabbing =
            vyIndex < -Config::TAP_Z_VELOCITY &&
            vyMiddle < -Config::TAP_Z_VELOCITY;

    if (state.twoFingerTapArmed && bothStabbing) {
        CGPoint clickPos = state.hasStablePos ? state.stablePos : state.smoothedPos;

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

void GestureDetector::handlePinch(const Hand &hand, HandState &state) {
    EventInjector::moveCursor(state.smoothedPos);
    detectZoom(hand, state);
}

bool GestureDetector::detectZoom(const Hand &hand, HandState &state) {
    float dist  = hand.pinchDistance();
    float pinch = hand.pinch();

    if (state.prevPinchDistance < 0.0f) {
        state.prevPinchDistance = dist;
        state.prevPinchStrength = pinch;
        state.zoomAccum = 0.0f;
        state.zoomedOutThisPinch = false;
        state.zoomedInThisPinch = false;
        return false;
    }

    float delta     = dist - state.prevPinchDistance;
    float pinchDrop = state.prevPinchStrength - pinch;
    state.prevPinchDistance = dist;
    state.prevPinchStrength = pinch;


    if (pinch < Config::ZOOM_PINCH_MIN) return false;

    if (pinchDrop > Config::ZOOM_RELEASE_DROP) {
        state.zoomAccum *= 0.5f;
        return false;
    }

    if (std::abs(delta) > Config::PINCH_MAX_DELTA) return false;

    float capped = std::clamp(delta, -Config::ZOOM_FRAME_CAP, Config::ZOOM_FRAME_CAP);
    state.zoomAccum += capped;

    if (frameCount_ - state.lastZoomFrame < 4) return false;

    if (state.zoomAccum >= Config::ZOOM_STEP_MM) {
        state.zoomAccum = 0.0f;
        state.lastZoomFrame = frameCount_;
        state.zoomedInThisPinch = true;
        if (state.hasStablePos) EventInjector::moveCursor(state.stablePos);
        EventInjector::zoom(+1.0f, 1);
        if (Config::DEBUG_GESTURES) std::cerr << "[fire] ZOOM IN\n";
        return true;
    }
    if (state.zoomAccum <= -Config::ZOOM_STEP_MM) {         // net squeeze → zoom out
        state.zoomAccum = 0.0f;
        state.lastZoomFrame = frameCount_;
        state.zoomedOutThisPinch = true;
        if (state.hasStablePos) EventInjector::moveCursor(state.stablePos);
        EventInjector::zoom(-1.0f, 1);
        if (Config::DEBUG_GESTURES) std::cerr << "[fire] ZOOM OUT\n";
        return true;
    }

    if (state.zoomedOutThisPinch &&
        dist < Config::ZOOM_CONTINUE_TIGHT_MM &&
        frameCount_ - state.lastZoomFrame >= Config::ZOOM_CONTINUE_FRAMES) {
        state.lastZoomFrame = frameCount_;
        if (state.hasStablePos) EventInjector::moveCursor(state.stablePos);
        EventInjector::zoom(-1.0f, 1);
        if (Config::DEBUG_GESTURES) std::cerr << "[fire] ZOOM OUT (hold at floor)\n";
        return true;
    }

    if (state.zoomedInThisPinch &&
        dist > Config::ZOOM_CONTINUE_WIDE_MM &&
        frameCount_ - state.lastZoomFrame >= Config::ZOOM_CONTINUE_FRAMES) {
        state.lastZoomFrame = frameCount_;
        if (state.hasStablePos) EventInjector::moveCursor(state.stablePos);
        EventInjector::zoom(+1.0f, 1);
        if (Config::DEBUG_GESTURES) std::cerr << "[fire] ZOOM IN (hold at ceiling)\n";
        return true;
    }
    return false;
}

bool GestureDetector::detectRotate(const Hand &hand, HandState &state) {
    Vector3 v = hand.fingerTip(4) - hand.fingerTip(1);
    if (v.length() < Config::ROTATE_MIN_SPAN) {
        state.hasPrevAngle = false;
        return false;
    }
    float angle = v.xzAngle();

    if (!state.hasPrevAngle) {
        state.prevIndexMiddleAngle = angle;
        state.hasPrevAngle = true;
        state.rotAccumDeg = 0.0f;
        return false;
    }

    float delta = angle - state.prevIndexMiddleAngle;
    if (delta >  M_PI) delta -= 2.0f * M_PI;
    if (delta < -M_PI) delta += 2.0f * M_PI;
    state.prevIndexMiddleAngle = angle;

    state.rotAccumDeg += delta * Config::ROTATE_SCALE;

    if (state.rotAccumDeg > Config::ROTATE_STEP_DEG) {
        EventInjector::rotate(+1.0f, 1);
        state.rotAccumDeg = 0.0f;
        if (Config::DEBUG_GESTURES) std::cerr << "[fire] ROTATE +\n";
        return true;
    }
    if (state.rotAccumDeg < -Config::ROTATE_STEP_DEG) {
        EventInjector::rotate(-1.0f, 1);
        state.rotAccumDeg = 0.0f;
        if (Config::DEBUG_GESTURES) std::cerr << "[fire] ROTATE -\n";
        return true;
    }
    return false;
}

void GestureDetector::handleOpenHand(const Hand &hand, HandState &state) {
    detectRotate(hand, state);
}


void GestureDetector::handleFist(const Hand &hand, HandState &state) {
    if (!state.dragging) {
        if (hand.grab() > Config::GRAB_ON_THRESHOLD) {
            ++state.grabRisingFrames;
            state.grabFallingFrames = 0;
            if (state.grabRisingFrames >= Config::GRAB_STABILITY_FRAMES) {
                state.dragging = true;
                state.dragAnchor = state.smoothedPos;
                state.dragPalmStart = hand.palmPosition();
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
        CGPoint target = state.dragAnchor;
        if (state.hasDragPalmStart) {
            float dx = (hand.palmPosition().x - state.dragPalmStart.x)
                       * Config::CURSOR_SCALE_X;
            float dz = (hand.palmPosition().z - state.dragPalmStart.z)
                       * Config::CURSOR_SCALE_Y;
            target.x = std::clamp(state.dragAnchor.x + dx, 0.0, (double) screenW_ - 1);
            target.y = std::clamp(state.dragAnchor.y + dz, 0.0, (double) screenH_ - 1);
        }

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
        state.smoothedPos = target;
    }
}

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

void GestureDetector::resetPinchState(HandState &state) {
    state.hasPrevAngle = false;
    state.prevIndexMiddleAngle = 0.0f;
}