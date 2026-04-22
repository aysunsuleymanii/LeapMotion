#pragma once

namespace Config {

    // ── Air cursor ──────────────────────────────────────────────────────────
    // Leap reports fingertip positions in mm relative to the device.
    // The tracking volume is roughly ±250 mm wide. To map that range onto
    // a ~1440 px wide screen we need a scale near 5.0.
    constexpr float CURSOR_SCALE_X     = 5.0f;
    constexpr float CURSOR_SCALE_Y     = 5.0f;

    // ── Scroll ──────────────────────────────────────────────────────────────
    // Palm Y/X velocity (mm/s) → scroll pixels per event.
    constexpr float SCROLL_SENSITIVITY = 0.05f;
    // Minimum palm speed (mm/s) to register a scroll tick.
    constexpr float SCROLL_MIN_SPEED   = 80.0f;

    // ── Pinch / Zoom ────────────────────────────────────────────────────────
    // Zoom is driven by thumb↔index distance (the SDK's pinch_distance, in mm).
    // Scale: mm of change per frame → CGEvent zoom magnification units.
    constexpr float PINCH_SCALE        = 0.015f;
    // Noise floor on pinch distance delta (mm/frame).
    constexpr float PINCH_NOISE_FLOOR  = 1.5f;

    // ── Rotation ────────────────────────────────────────────────────────────
    // Minimum angle delta (radians) per frame to emit a rotate event.
    constexpr float ROTATE_MIN_DELTA   = 0.02f;
    // Scale: radians → degrees (CGEvent rotation unit).
    constexpr float ROTATE_SCALE       = 57.296f;   // 180/π

    // ── Tap / Click ─────────────────────────────────────────────────────────
    // A tap = fingertip Z-velocity spike toward the screen, with the palm
    // mostly stationary. 1 finger → left click, 2 fingers → right click.
    constexpr float TAP_Z_VELOCITY     = 350.0f;    // mm/s toward the screen
    constexpr float TAP_PALM_MAX_SPEED = 120.0f;    // palm must be roughly still
    constexpr int   TAP_COOLDOWN_FRAMES = 12;

    // Double-tap window for smart zoom. Leap runs ~120 Hz → 48 frames ≈ 400 ms.
    constexpr int   SMART_ZOOM_MAX_GAP_FRAMES = 48;

    // ── Swipe ───────────────────────────────────────────────────────────────
    // Palm X velocity (mm/s) required to fire a page-turn swipe.
    constexpr float SWIPE_VELOCITY_THRESHOLD = 600.0f;
    // Cooldown frames after a swipe fires (prevents re-fire on the same motion).
    constexpr int   SWIPE_COOLDOWN_FRAMES    = 30;

    // ── Grab / Drag / Drop (3D-exclusive) ───────────────────────────────────
    // grab_strength = 0 (open) → 1 (closed fist). Hysteresis prevents
    // a partially closed hand from toggling drag on/off every frame.
    constexpr float GRAB_ON_THRESHOLD   = 0.85f;   // rising edge → start drag
    constexpr float GRAB_OFF_THRESHOLD  = 0.55f;   // falling edge → drop
    // Require the new state for N consecutive frames before committing.
    constexpr int   GRAB_STABILITY_FRAMES = 3;

    // ── Pose classifier ─────────────────────────────────────────────────────
    // If grab_strength is above this, the hand is treated as a fist regardless
    // of how many digits the SDK marks "extended" (noisy at high grab).
    constexpr float FIST_GRAB_MIN       = 0.75f;

} // namespace Config