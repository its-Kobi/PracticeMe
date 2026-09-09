# Development -- KRecorder Architecture
## Modules
- Mod init (src/main.cpp)
- Settings (src/Settings.cpp) -- wrapper over Mod::getSettingValue
- Hotkey (src/Hotkey.cpp) -- CCKeyboardDispatcher + Win32 GetAsyncKeyState debounce
- Overlay (src/Overlay.cpp) -- CCLayerColor red dot Z=9999, excluded from capture because capture hides overlay during backbuffer copy
- Capture (IFrameCapture -> DesktopDuplication) -- IDXGIOutputDuplication, window rect only
- Encoder (IVideoEncoder -> WMFEncoder, FFmpegEncoder stub) -- IMFSinkWriter, hardware MFT first
- RecordingManager -- queue<Frame> bounded 120, drop-oldest, worker thread, state Idle/Recording/Error
- FileUtils -- Videos/KRecorder, timestamp filenames, free space check
## Pipeline
GD render -> onFrameCaptured (post-present, overlay hidden) -> push queue -> worker pops -> encode -> MP4
Main thread never blocks.
## Audio
Implement IAudioCapture (WASAPI loopback) and multiplex.
## Replacing Encoder
Implement IVideoEncoder and register in EncoderFactory.
