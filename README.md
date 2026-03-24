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
| Tap to Click | One-finger tap (index fingertip quick extend-retract) | Fingertip velocity spike + dwell < 150 ms |
| Right-Click | Two-finger tap (index + middle quick double-tap) | Two fingertip velocity spikes, simultaneous, dwell < 150 ms |
| Scroll | Two-finger slide up/down (index + middle extended, hand moves on Y axis) | hand.palm.velocity.y mapped to kCGScrollEventUnit |
| Zoom (Pinch) | Two-finger pinch (thumb + index distance changing) | Distance delta between thumb tip and index tip |
| Rotate | Two-finger rotate (thumb + index rotate around midpoint) | Angle delta of thumb–index vector around palm normal |
| Swipe Between Pages | Two-finger swipe left/right (fast horizontal motion) | hand.palm.velocity.x above threshold, short duration |
| Smart Zoom | Two-finger double-tap (pinch gesture twice within 400 ms) | Two pinch-open events in rapid succession |


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
