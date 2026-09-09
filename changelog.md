# v1.0.0
- Initial release
- Async capture pipeline (D3D11 Desktop Duplication -> queue -> WMF encoder)
- Overlay indicator excluded from capture
- Hotkey toggle (F9), configurable
- Hardware encoding with software fallback
- Graceful error handling

# v1.0.1
- Fix F9 hotkey debounce and polling (was never triggered)
- Default output now Desktop/KRecorder -> .mp4
- Added main menu KRecorder button (MenuLayer hook)
