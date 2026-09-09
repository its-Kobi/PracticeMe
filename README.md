# KRecorder -- Geometry Dash 2.2 Geode Recording Mod
Lightweight built-in Geometry Dash recorder without OBS.

## Features
- One-key toggle (F9 default, remappable)
- Minimal overlay indicator (red dot) NOT in video
- Asynchronous pipeline: Render thread -> queue -> encoder thread -> file
- Hardware encoding (NVENC/AMF/QSV via Media Foundation) with software fallback
- Configurable FPS/resolution/bitrate/output dir

## Requirements
- GD 2.2 (2.2081), Geode 4.8.0+, Windows 10/11

## How to Build
```bat
build.bat
```
Requires CMake 3.21+, Ninja or VS2022, Git.

## Install
Copy `KRecorder.geode` into `geode/mods/` or install via Geode.

## Use
1. Launch GD 2. Open KRecorder settings 3. Enter level 4. Press F9 -> red dot 5. Press F9 again -> saved to Videos/KRecorder/

## Limitations
- Audio capture stubbed (video-only v1.0.0)
- High res+high FPS may drop frames (handled)

## Backend
Windows Media Foundation. FFmpegEncoder abstraction ready for eclipse.ffmpeg-api.
