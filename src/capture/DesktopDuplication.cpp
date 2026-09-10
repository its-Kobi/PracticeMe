#include "IFrameCapture.hpp"
#ifdef _WIN32
#include <windows.h>
#include <Geode/Geode.hpp>
#include <chrono>
#pragma comment(lib,"gdi32.lib")
#endif

#ifdef _WIN32
static BOOL CALLBACK enumFindGD(HWND hwnd, LPARAM lParam) {
    char title[256] = {}; GetWindowTextA(hwnd, title, sizeof(title));
    char cls[64] = {}; GetClassNameA(hwnd, cls, sizeof(cls));
    if (strstr(title, "Geometry Dash") || strstr(cls, "GLFW")) {
        if (IsWindowVisible(hwnd) && !IsIconic(hwnd)) {
            RECT rc{}; GetClientRect(hwnd, &rc);
            if (rc.right > 100 && rc.bottom > 100) {
                *(HWND*)lParam = hwnd;
                return FALSE;
            }
        }
    }
    return TRUE;
}
static HWND findGDWindow() {
    HWND hwnd = FindWindowA(nullptr, "Geometry Dash");
    if (hwnd && IsWindowVisible(hwnd)) return hwnd;
    HWND found = nullptr;
    EnumWindows(enumFindGD, (LPARAM)&found);
    if (found) return found;
    hwnd = GetForegroundWindow();
    if (hwnd) {
        char title[256] = {}; GetWindowTextA(hwnd, title, sizeof(title));
        if (strstr(title, "Geometry Dash") || strstr(title, "GeometryDash")) return hwnd;
    }
    return FindWindowA("GLFW30", nullptr);
}
#endif

// Game-only capture: captures Geometry Dash window client area only, never desktop.
// Fast path: window DC BitBlt + GetDIBits, no DXGI desktop duplication, no CreateTexture2D per frame.
class DesktopDuplication : public IFrameCapture {
public:
    bool init(int w,int h) override {
        m_reqW = w; m_reqH = h;
#ifdef _WIN32
        // If user requested 0,0 use native window size (no scaling, fastest)
        if (w==0 || h==0) geode::log::info("Capture initialized: window-only GDI BitBlt native size");
        else geode::log::info("Capture initialized: window-only GDI BitBlt (req {}x{})", w, h);
        m_hMem = nullptr; m_hBmp = nullptr; m_cachedW=0; m_cachedH=0;
#endif
        return true;
    }

    std::optional<Frame> capture() override {
#ifdef _WIN32
        auto t0 = std::chrono::steady_clock::now();
        HWND hwnd = findGDWindow();
        if (!hwnd) {
            // Game not found -> do not capture desktop
            return std::nullopt;
        }
        if (IsIconic(hwnd)) return std::nullopt; // minimized

        RECT cr{}; GetClientRect(hwnd, &cr);
        int winW = cr.right - cr.left;
        int winH = cr.bottom - cr.top;
        if (winW <= 0 || winH <= 0) return std::nullopt;
        // Reject if window is too small or not visible
        if (!IsWindowVisible(hwnd)) return std::nullopt;

        // Game-only: BitBlt from window client DC, not desktop - reuse HDC/bitmap for speed (no per-frame alloc)
        HDC hWindowDC = GetDC(hwnd);
        if (!hWindowDC) return std::nullopt;

        int targetW = m_reqW > 0 ? m_reqW : winW;
        int targetH = m_reqH > 0 ? m_reqH : winH;
        if (targetW % 2) targetW--; if (targetH % 2) targetH--;

        // Reuse compatible DC and bitmap if size unchanged (avoids 70ms CreateTexture2D-like cost)
        if (!m_hMem) m_hMem = CreateCompatibleDC(hWindowDC);
        if (!m_hMem) { ReleaseDC(hwnd, hWindowDC); return std::nullopt; }
        if (winW != m_cachedW || winH != m_cachedH) {
            if (m_hBmp) { DeleteObject(m_hBmp); m_hBmp = nullptr; }
            m_hBmp = CreateCompatibleBitmap(hWindowDC, winW, winH);
            m_cachedW = winW; m_cachedH = winH;
            if (m_hBmp) SelectObject(m_hMem, m_hBmp);
        }
        if (!m_hBmp) { ReleaseDC(hwnd, hWindowDC); return std::nullopt; }

        auto t1 = std::chrono::steady_clock::now();
        BOOL blt = BitBlt(m_hMem, 0, 0, winW, winH, hWindowDC, 0, 0, SRCCOPY);
        auto t2 = std::chrono::steady_clock::now();
        if (!blt) {
            ReleaseDC(hwnd, hWindowDC);
            return std::nullopt;
        }

        BITMAPINFO bmi{}; bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = winW;
        bmi.bmiHeader.biHeight = -winH;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        // Reuse vector to avoid per-frame 8MB alloc
        thread_local std::vector<uint8_t> raw;
        raw.resize((size_t)winW * winH * 4);
        int lines = GetDIBits(m_hMem, m_hBmp, 0, winH, raw.data(), &bmi, DIB_RGB_COLORS);
        auto t3 = std::chrono::steady_clock::now();

        ReleaseDC(hwnd, hWindowDC);

        if (lines == 0) return std::nullopt;

        Frame f;
        f.timestampUs = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();

        if (targetW == winW && targetH == winH) {
            f.width = winW; f.height = winH;
            f.data = std::move(raw);
        } else {
            // Only scale if requested size differs - this is extra cost, avoid if possible by setting 0,0 for native
            f.width = targetW; f.height = targetH;
            f.data.resize((size_t)targetW * targetH * 4);
            for (int y = 0; y < targetH; ++y) {
                int sy = y * winH / targetH;
                for (int x = 0; x < targetW; ++x) {
                    int sx = x * winW / targetW;
                    memcpy(f.data.data() + ((size_t)y * targetW + x) * 4,
                           raw.data() + ((size_t)sy * winW + sx) * 4, 4);
                }
            }
        }

        auto t4 = std::chrono::steady_clock::now();
        auto capMs = std::chrono::duration_cast<std::chrono::microseconds>(t4 - t0).count() / 1000.0;
        auto bltMs = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count() / 1000.0;
        auto dibMs = std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2).count() / 1000.0;
        // Diagnostics: log slow captures and throttled periodic stats
        static int s_count = 0;
        static auto s_lastLog = std::chrono::steady_clock::now();
        s_count++;
        bool slow = capMs > 15.0;
        if (slow || s_count % 60 == 0) {
            auto now = std::chrono::steady_clock::now();
            double elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - s_lastLog).count() / 1000.0;
            double fps = s_count / (elapsed > 0 ? elapsed : 1);
            geode::log::info("KRecorder: capture {}x{} time={:.1f}ms (blt={:.1f} dib={:.1f}) fps~{:.1f} queue={}", f.width, f.height, capMs, bltMs, dibMs, fps, 0);
            if (s_count % 300 == 0) s_lastLog = now;
        }
        if (slow) {
            geode::log::warn("KRecorder: slow capture {:.1f}ms - may cause 10-13 fps", capMs);
        }

        return f;
#else
        return std::nullopt;
#endif
    }

    void shutdown() override {
#ifdef _WIN32
        if (m_hBmp) { DeleteObject(m_hBmp); m_hBmp=nullptr; }
        if (m_hMem) { DeleteDC(m_hMem); m_hMem=nullptr; }
        m_cachedW=0; m_cachedH=0;
#endif
    }

private:
    int m_reqW = 0, m_reqH = 0;
#ifdef _WIN32
    HDC m_hMem = nullptr;
    HBITMAP m_hBmp = nullptr;
    int m_cachedW = 0, m_cachedH = 0;
#endif
};

// Factory used by RecordingManager
extern "C" IFrameCapture* createDesktopDuplicationCapture() {
    return new DesktopDuplication();
}
