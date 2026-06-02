#pragma once

namespace Config {
    constexpr bool DEBUG_GESTURES = true;

    // ── Air cursor ──────────────────────────────────────────────────────────
    constexpr float CURSOR_SCALE_X = 5.0f;
    constexpr float CURSOR_SCALE_Y = 5.0f;

    // ── Scroll ──────────────────────────────────────────────────────────────
    constexpr float SCROLL_SENSITIVITY = 0.1f;
    constexpr float SCROLL_MIN_SPEED = 80.0f;

    // ── Pinch / Zoom ────────────────────────────────────────────────────────
    constexpr float PINCH_SCALE = 0.003f;
    constexpr float PINCH_NOISE_FLOOR = 3.0f;

    // ── Rotation ────────────────────────────────────────────────────────────
    constexpr float ROTATE_MIN_DELTA = 0.05f;
    constexpr float ROTATE_SCALE = 57.296f;

    // ── Tap / Click ─────────────────────────────────────────────────────────
    constexpr float TAP_Z_VELOCITY = 150.0f;
    constexpr float TAP_PALM_MAX_SPEED = 80.0f;
    constexpr int TAP_COOLDOWN_FRAMES = 20;

    constexpr int SMART_ZOOM_MAX_GAP_FRAMES = 48;

    // ── Swipe ───────────────────────────────────────────────────────────────
    constexpr float SWIPE_VELOCITY_THRESHOLD = 900.0f;
    constexpr int SWIPE_SUSTAIN_FRAMES = 3;
    constexpr int SWIPE_COOLDOWN_FRAMES = 45;

    // ── Grab / Drag / Drop (3D-exclusive) ───────────────────────────────────
    constexpr float GRAB_ON_THRESHOLD = 0.55f;
    constexpr float GRAB_OFF_THRESHOLD = 0.30f;
    constexpr int GRAB_STABILITY_FRAMES = 3;

    // ── Pose classifier ─────────────────────────────────────────────────────
    constexpr int POSE_STABILITY_FRAMES = 6;
    constexpr float PINCH_POSE_MIN_STRENGTH = 0.80f;
    constexpr float PINCH_POSE_MAX_DIST = 40.0f;
    constexpr float FIST_GRAB_MIN = 0.65f;
    constexpr float FIST_MAX_PINCH = 0.90f;
    constexpr float CURSOR_OFFSET_X = -100.0f;

    constexpr float LEAP_X_MIN = -150.0f;
    constexpr float LEAP_X_MAX = 150.0f;
    constexpr float LEAP_Z_MIN = -120.0f;
    constexpr float LEAP_Z_MAX = 120.0f;
} // namespace Config
