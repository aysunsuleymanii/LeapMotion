# LeapMotion Gesture Controller
A macOS application that translates Ultraleap (Leap Motion) hand tracking into native system-wide multi-touch gestures and 3D-exclusive interactions — replacing the trackpad entirely, for any application.  

## Overview
This project maps hand and finger gestures detected by the Ultraleap hand tracking sensor to macOS input events injected at the system level via CGEvent (ApplicationServices framework). Because events are posted to the HID event stream, they work across all applications without any app-specific integration.

## High Level Architecture
<img width="5997" height="1720" alt="HighLevelArch" src="https://github.com/user-attachments/assets/c7c8b134-acee-42b8-aa0d-1b5c4f3763b0" />

## Gesture Mapping
### Gesture to Trackpad Mapping

| GestureTrackpad Action | Equivalent Hand Pose | Detection Signal |
|----------------------|--------------------|------------------|
| Cursor Movement | One-finger pointing (index extended, stable hand) | Continuous fingertip position mapping (index tip world → screen space) |
| Tap to Click | One-finger tap (index fingertip quick extend-retract) | Fingertip velocity spike + dwell < 150 ms |
| Right-Click | Two-finger tap (index + middle quick double-tap) | Two fingertip velocity spikes, simultaneous, dwell < 150 ms |
| Scroll | Two-finger slide up/down (index + middle extended, hand moves on Y axis) | hand.palm.velocity.y mapped to kCGScrollEventUnit |
| Zoom (Pinch) | Two-finger pinch (thumb + index distance changing) | Distance delta between thumb tip and index tip |
| Rotate | Two-finger rotate (thumb + index rotate around midpoint) | Angle delta of thumb–index vector around palm normal |
| Swipe Between Pages | Two-finger swipe left/right (fast horizontal motion) | hand.palm.velocity.x above threshold, short duration |
| Drag & Drop | Fist → grab → move → release | Grab state triggered by fist closure (grab threshold), object follows palm position while GRAB active, release on hand open (grab off threshold), movement mapped to continuous cursor drag |


## Build & Run
Prerequisites
- macOS 12+  
- Ultraleap Hand Tracking Service installed (download)  
- CMake 3.16+  
- Xcode Command Line Tools: xcode-select --install  

### Build
In the project directory:
```
mkdir build && cd build
cmake ..
make
```

### Run
```
./leap
```


## Gesture Testing Guide

Follow this order and test each gesture separately.

---

### 1. Cursor Movement (Index Finger)

**Gesture:**
- Only index finger extended
- Move hand freely

**Test:**
- Open TextEdit or any app
- Move hand left/right/up/down → cursor follows
- Cursor must reach all screen edges (Dock, menu bar, etc.)

**Logs:**
- `pose=OneFinger`
- No `[fire]` event for movement

**Expected Result:**
- Smooth tracking
- Slight jitter allowed
- Stable when hand is still

---

### 2. Left Click

**Gesture:**
- Index finger extended
- Brief downward tap motion

**Test:**
- Hover over desktop folder
- Tap gesture → folder selected

**Logs:**
- `[fire] LEFT CLICK at (X, Y)`

---

### 3. Right Click

**Gesture:**
- Index + middle finger (V shape)
- Press down

**Test:**
- Right click on desktop background
- Context menu appears

**Logs:**
- `[fire] RIGHT CLICK`

---

### 4. Scroll

**Gesture:**
- Index + middle finger extended
- Move hand forward/backward

**Test:**
- Open Safari (long page)
- Pull hand toward you → scroll down
- Push away → scroll up

**Logs:**
- `[fire] SCROLL vY=XXX`

---

### 5. Swipe

**Gesture:**
- Index + middle finger extended
- Fast horizontal movement

**Test:**
- Safari tabs or Photos app
- Swipe left/right → change page/photo

**Logs:**
- `[fire] SWIPE LEFT / SWIPE RIGHT`

**Important:**
- Slow movement = scroll
- Fast movement (~900 mm/s+) = swipe

---

### 6. Zoom (Pinch)

**Gesture:**
- Thumb + index finger pinch

**Test:**
- Safari webpage
- Spread fingers → zoom in
- Pinch → zoom out

**Logs:**
- `[fire] ZOOM delta=+X.X`
- `[fire] ZOOM delta=-X.X`

---

### 7. Rotate

**Gesture:**
- Open your hand flat (all fingers extended)
- Rotate your hand like a dial / steering wheel (wrist rotation)

**Test:**
- Open an image in Preview
- Turn hand right → image rotates right (Cmd + R)
- Turn hand left → image rotates left (Cmd + L)

**Logs:**
- `[fire] ROTATE delta=+X.X`

---

### 8. Drag & Drop (Fist Gesture)

**Gesture:**
- Make fist → drag starts
- Move hand while closed
- Open hand → drop

**Test:**
- Create file on Desktop
- Create folder named `test`
- Hover file → make fist → drag starts
- Move into folder
- Open hand → drop completes

**Logs:**
- `[fire] DRAG START at (X, Y)`
- `[fire] DROP at (X, Y)`

**Important:**
If drag works correctly, core interaction system is fully functional.

---

## Final Test Checklist

| # | Gesture | Test Area | Expected Result |
|--|--------|----------|----------------|
| 1 | Cursor Movement | Entire screen | Smooth tracking |
| 2 | Left Click | Desktop folder | Select item |
| 3 | Right Click | Desktop area | Context menu |
| 4 | Scroll | Safari page | Page scroll |
| 5 | Swipe | Photos/Safari | Page switching |
| 6 | Zoom | Safari page | Zoom in/out |
| 7 | Rotate | Preview image | Image rotation |
| 8 | Drag & Drop | Desktop files | File transfer |
