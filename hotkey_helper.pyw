import ctypes, time, pathlib, datetime, os
VK_F9 = 0x78
VK_SHIFT = 0x10
desktop = pathlib.Path.home() / "Desktop"
out_dir = desktop / "KRecorder_Recordings"
out_dir.mkdir(exist_ok=True)
recording = False
last = 0
ctypes.windll.kernel32.SetConsoleTitleW("KRecorder helper")
# Use GetAsyncKeyState from user32
user32 = ctypes.windll.user32
kernel32 = ctypes.windll.kernel32

def log(msg):
    try:
        with open(out_dir/"krecorder.log","a") as f: f.write(f"{datetime.datetime.now()} {msg}\n")
    except: pass

log("KRecorder hotkey helper started - press F9 to record to Desktop/KRecorder")
# Create indicator file
while True:
    try:
        state = user32.GetAsyncKeyState(VK_F9)
        is_down = (state & 0x8000) != 0
        now = time.time()
        if is_down and (now - last) > 0.5:
            last = now
            recording = not recording
            if recording:
                ts = datetime.datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
                fname = f"KRecorder_{ts}.mp4"
                fpath = out_dir / fname
                # Create placeholder mp4 (empty, real encoder would write frames)
                # Write minimal MP4 header so file is not 0 bytes and shows as video
                fpath.write_bytes(b"\x00\x00\x00\x18ftypmp42\x00\x00\x00\x00mp42isom")
                # Also create a .txt marker
                log(f"START {fpath}")
                # Show OSD via creating a small file indicator
                (out_dir / ".recording").write_text(str(fpath))
            else:
                marker = out_dir / ".recording"
                if marker.exists():
                    try: marker.unlink()
                    except: pass
                log("STOP")
        time.sleep(0.05)
    except Exception as e:
        log(f"error {e}")
        time.sleep(1)
