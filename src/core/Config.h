#pragma once

/**
 * @file Config.h
 * @brief Central tunable constants for every gesture.
 *
 * All thresholds, sensitivities, cooldowns and pose-classifier cutoffs live
 * here in one namespace so behaviour can be adjusted without touching logic.
 * Inline comments on each constant explain its effect and a sensible range.
 */
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
    // Accumulated pinch-distance change (mm) that triggers one zoom step. Higher
    // = LESS sensitive. 6 is a reliable middle ground; raise toward 8–10 if it
    // feels twitchy, lower toward 4 if it feels sluggish.
    constexpr float ZOOM_STEP_MM = 6.0f;
    // Max one frame may add to the accumulator (mm). MUST be < ZOOM_STEP_MM so a
    // single fast jump (e.g. the hand snapping open) can't fire a zoom alone.
    constexpr float ZOOM_FRAME_CAP = 4.0f;
    // A spread only counts as zoom-in while pinch is within this of its peak.
    // Past that the pinch is easing = release starting, so outward motion is
    // ignored. Small, so genuine pinch noise during a real zoom-in still passes.
    constexpr float ZOOM_SPREAD_TOL = 0.03f;
    // Max believable pinch-distance change in one frame (mm). Anything larger is
    // a grab/release transient or glitch, not a deliberate zoom — ignored.
    constexpr float PINCH_MAX_DELTA = 20.0f;
    // Min pinch strength to zoom at all.
    constexpr float ZOOM_PINCH_MIN = 0.85f;
    // Per-frame DROP in pinch strength that means a fast snap-open (release).
    // Kept high on purpose: a normal zoom-in spread also lowers pinch, just
    // gradually, so only a sharp single-frame collapse should suppress zoom.
    constexpr float ZOOM_RELEASE_DROP = 0.12f;
    // HOLD-TO-CONTINUE zoom out: when pinch distance is squeezed below this (mm)
    // — i.e. near the finger floor with no travel left — holding the tight pinch
    // keeps zooming out, one step every ZOOM_CONTINUE_FRAMES frames.
    constexpr float ZOOM_CONTINUE_TIGHT_MM = 7.0f;
    // HOLD-TO-CONTINUE zoom in: when spread wider than this (mm) and the user
    // actively spread to zoom in, holding the wide pinch keeps zooming in (the
    // pose would otherwise cap zoom-in once you spread too far). Kept above a
    // normal resting pinch so just holding a pinch doesn't drift into it.
    constexpr float ZOOM_CONTINUE_WIDE_MM  = 20.0f;
    constexpr int   ZOOM_CONTINUE_FRAMES   = 20;
    // How far pinch strength may fall below its peak before the rest of the pinch
    // is treated as a release (freezing zoom). Catches slow releases where the
    // per-frame drop is too small to notice.
    constexpr float ZOOM_RELEASE_MARGIN = 0.05f;

    // ── Rotation ────────────────────────────────────────────────────────────
    // Per-frame angle change (radians) before rotation counts. Raised so small
    // wrist wobble while pinching/zooming doesn't fire rotate (= Cmd+R / Reload
    // in Safari). Only a clear, deliberate twist should trigger it.
    constexpr float ROTATE_MIN_DELTA = 0.15f;     // (legacy, unused)
    constexpr float ROTATE_SCALE = 57.296f;       // radians → degrees
    // Wrist turn (degrees) accumulated before one rotate step fires. Each step
    // sends Cmd+R / Cmd+L = a 90° image rotation in Preview, so this is the
    // hand turn per 90° image rotation. Larger = less sensitive.
    constexpr float ROTATE_STEP_DEG  = 45.0f;
    // Min index→pinky tip span (mm) for the hand to count as open enough to
    // rotate. Stops a half-open/closing hand from emitting rotations.
    constexpr float ROTATE_MIN_SPAN  = 40.0f;

    // ── Tap / Click ─────────────────────────────────────────────────────────
    constexpr float TAP_Z_VELOCITY = 150.0f;
    constexpr float TAP_PALM_MAX_SPEED = 80.0f;
    constexpr int TAP_COOLDOWN_FRAMES = 20;

    constexpr int SMART_ZOOM_MAX_GAP_FRAMES = 48;

    // ── Swipe ───────────────────────────────────────────────────────────────
    // A real horizontal flick peaks around 500–1000 mm/s for only 1–2 frames,
    // so the old 900 mm/s × 3-frame gate almost never triggered. Lowered so a
    // deliberate two-finger swipe reliably fires one back/forward navigation.
    constexpr float SWIPE_VELOCITY_THRESHOLD = 450.0f;  // mm/s on the X axis
    constexpr int SWIPE_SUSTAIN_FRAMES = 2;             // consecutive frames
    constexpr int SWIPE_COOLDOWN_FRAMES = 45;           // one swipe per ~0.4 s

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