#include "IFrameCapture.hpp"
#ifdef _WIN32
#include <d3d11.h>
#include <dxgi1_2.h>
#include <windows.h>
#include <wrl/client.h>
#include <Geode/Geode.hpp>
#include <chrono>
#include <algorithm>

#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"gdi32.lib")
#endif

#ifdef _WIN32
// Find Geometry Dash window - GLFW title is "Geometry Dash"
static HWND findGDWindow() {
    HWND hwnd = FindWindowA(nullptr, "Geometry Dash");
    if (hwnd) return hwnd;
    // fallback: active foreground if its process is GD
    hwnd = GetForegroundWindow();
    if (hwnd) {
        char title[256] = {};
        GetWindowTextA(hwnd, title, sizeof(title));
        if (strstr(title, "Geometry Dash") || strstr(title, "GeometryDash")) return hwnd;
    }
    hwnd = FindWindowA("GLFW30", nullptr);
    return hwnd;
}

static std::wstring toWide(const std::string& s) {
    if (s.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring w(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), len);
    if (!w.empty() && w.back() == L'\0') w.pop_back();
    return w;
}
#endif

class DesktopDuplication : public IFrameCapture {
public:
    bool init(int w,int h) override {
        m_reqW = w; m_reqH = h;
#ifdef _WIN32
        D3D_FEATURE_LEVEL lvl;
        HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
            0, nullptr, 0, D3D11_SDK_VERSION, &m_dev, &lvl, &m_ctx);
        if (FAILED(hr)) {
            geode::log::warn("DesktopDuplication: D3D11CreateDevice failed {}", (int)hr);
            m_useGDI = true;
            return true; // allow GDI fallback
        }
        Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDev;
        if (FAILED(m_dev.As(&dxgiDev))) { m_useGDI = true; return true; }
        Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
        if (FAILED(dxgiDev->GetAdapter(&adapter))) { m_useGDI = true; return true; }
        Microsoft::WRL::ComPtr<IDXGIOutput> out;
        if (FAILED(adapter->EnumOutputs(0, &out))) { m_useGDI = true; return true; }
        Microsoft::WRL::ComPtr<IDXGIOutput1> out1;
        if (FAILED(out.As(&out1))) { m_useGDI = true; return true; }
        hr = out1->DuplicateOutput(m_dev.Get(), &m_dup);
        if (FAILED(hr)) {
            geode::log::warn("DuplicateOutput failed {} - using GDI fallback", (int)hr);
            m_dup = nullptr;
            m_useGDI = true;
            return true;
        }
        geode::log::info("Capture initialized: DXGI duplication (req {}x{})", w, h);
        m_useGDI = false;
        return true;
#else
        return true;
#endif
    }

    std::optional<Frame> capture() override {
#ifdef _WIN32
        HWND hwnd = findGDWindow();
        RECT wr{};
        int winW = 0, winH = 0;
        bool hasWindow = false;
        if (hwnd && GetWindowRect(hwnd, &wr)) {
            // use client rect for game content (without borders)
            RECT cr{}; GetClientRect(hwnd, &cr);
            POINT pt{0,0}; ClientToScreen(hwnd, &pt);
            wr.left = pt.x; wr.top = pt.y;
            wr.right = pt.x + cr.right;
            wr.bottom = pt.y + cr.bottom;
            winW = cr.right;
            winH = cr.bottom;
            if (winW > 0 && winH > 0) hasWindow = true;
        }

        // Try DXGI first if available
        if (!m_useGDI && m_dup) {
            auto frame = captureDXGI(hasWindow, wr, winW, winH);
            if (frame) return frame;
            // fallthrough to GDI on timeout/failure
        }
        // GDI fallback - BitBlt window
        return captureGDI(hasWindow, wr, winW, winH);
#else
        return std::nullopt;
#endif
    }

    void shutdown() override {
#ifdef _WIN32
        if (m_dup) { m_dup->Release(); m_dup = nullptr; }
        m_ctx.Reset();
        m_dev.Reset();
#endif
    }

private:
#ifdef _WIN32
    std::optional<Frame> captureDXGI(bool hasWindow, RECT wr, int winW, int winH) {
        Microsoft::WRL::ComPtr<IDXGIResource> res;
        DXGI_OUTDUPL_FRAME_INFO info{};
        HRESULT hr = m_dup->AcquireNextFrame(50, &info, &res);
        if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
            return std::nullopt;
        }
        if (FAILED(hr) || !res) {
            if (hr == DXGI_ERROR_ACCESS_LOST || hr == DXGI_ERROR_INVALID_CALL) {
                geode::log::warn("DXGI AcquireNextFrame lost {}", (int)hr);
            }
            if (res) m_dup->ReleaseFrame();
            return std::nullopt;
        }
        Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
        res.As(&tex);
        D3D11_TEXTURE2D_DESC desc{}; tex->GetDesc(&desc);

        // Need staging texture to map
        D3D11_TEXTURE2D_DESC stageDesc = desc;
        stageDesc.Usage = D3D11_USAGE_STAGING;
        stageDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        stageDesc.BindFlags = 0;
        stageDesc.MiscFlags = 0;
        stageDesc.MipLevels = 1;
        stageDesc.ArraySize = 1;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> stage;
        hr = m_dev->CreateTexture2D(&stageDesc, nullptr, &stage);
        if (FAILED(hr)) { m_dup->ReleaseFrame(); return std::nullopt; }

        // If we have window rect, copy only that subresource region
        if (hasWindow) {
            D3D11_BOX box{};
            box.left = std::max<LONG>(0, wr.left);
            box.top = std::max<LONG>(0, wr.top);
            box.right = wr.right;
            box.bottom = wr.bottom;
            box.front = 0; box.back = 1;
            // Clamp to desktop size
            if (box.right > (LONG)desc.Width) box.right = desc.Width;
            if (box.bottom > (LONG)desc.Height) box.bottom = desc.Height;
            if (box.right <= box.left || box.bottom <= box.top) {
                m_dup->ReleaseFrame();
                return std::nullopt;
            }
            m_ctx->CopySubresourceRegion(stage.Get(), 0, 0, 0, 0, tex.Get(), 0, &box);
            // width/height for this cropped region
            winW = box.right - box.left;
            winH = box.bottom - box.top;
        } else {
            m_ctx->CopyResource(stage.Get(), tex.Get());
            winW = desc.Width;
            winH = desc.Height;
        }

        D3D11_MAPPED_SUBRESOURCE mapped{};
        hr = m_ctx->Map(stage.Get(), 0, D3D11_MAP_READ, 0, &mapped);
        if (FAILED(hr)) { m_dup->ReleaseFrame(); return std::nullopt; }

        Frame f;
        f.width = winW;
        f.height = winH;
        // Requested size handling: if init requested specific size and differs, do simple nearest-neighbor scale
        int targetW = m_reqW > 0 ? m_reqW : winW;
        int targetH = m_reqH > 0 ? m_reqH : winH;
        bool needScale = (targetW != winW || targetH != winH) && targetW > 0 && targetH > 0 && winW > 0 && winH > 0;

        f.timestampUs = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        f.data.resize((size_t)targetW * targetH * 4);

        if (!needScale) {
            // Direct copy, handling pitch
            uint8_t* dst = f.data.data();
            uint8_t* src = reinterpret_cast<uint8_t*>(mapped.pData);
            for (int y = 0; y < winH; ++y) {
                memcpy(dst + (size_t)y * winW * 4, src + (size_t)y * mapped.RowPitch, (size_t)winW * 4);
            }
        } else {
            // Nearest neighbor scale (fast, no extra deps)
            uint8_t* srcBase = reinterpret_cast<uint8_t*>(mapped.pData);
            // First copy to temp buffer
            std::vector<uint8_t> srcBuf((size_t)winW * winH * 4);
            for (int y = 0; y < winH; ++y) {
                memcpy(srcBuf.data() + (size_t)y * winW * 4, srcBase + (size_t)y * mapped.RowPitch, (size_t)winW * 4);
            }
            for (int y = 0; y < targetH; ++y) {
                int sy = y * winH / targetH;
                for (int x = 0; x < targetW; ++x) {
                    int sx = x * winW / targetW;
                    memcpy(f.data.data() + ((size_t)y * targetW + x) * 4,
                           srcBuf.data() + ((size_t)sy * winW + sx) * 4, 4);
                }
            }
            f.width = targetW;
            f.height = targetH;
        }

        m_ctx->Unmap(stage.Get(), 0);
        m_dup->ReleaseFrame();
        return f;
    }

    std::optional<Frame> captureGDI(bool hasWindow, RECT wr, int winW, int winH) {
        HWND hwnd = findGDWindow();
        if (!hwnd || !hasWindow) {
            // Fallback to desktop
            int sw = GetSystemMetrics(SM_CXSCREEN);
            int sh = GetSystemMetrics(SM_CYSCREEN);
            wr.left = 0; wr.top = 0; wr.right = sw; wr.bottom = sh;
            winW = sw; winH = sh;
            hwnd = GetDesktopWindow();
        }
        HDC hScreen = GetDC(hwnd ? hwnd : nullptr);
        if (!hScreen) return std::nullopt;
        HDC hMem = CreateCompatibleDC(hScreen);
        if (!hMem) { ReleaseDC(hwnd, hScreen); return std::nullopt; }

        int targetW = m_reqW > 0 ? m_reqW : winW;
        int targetH = m_reqH > 0 ? m_reqH : winH;

        HBITMAP hbmp = CreateCompatibleBitmap(hScreen, winW, winH);
        if (!hbmp) { DeleteDC(hMem); ReleaseDC(hwnd, hScreen); return std::nullopt; }
        HGDIOBJ old = SelectObject(hMem, hbmp);

        // For window capture, BitBlt from window DC (client area)
        // hwnd DC from GetDC includes client; for desktop fallback use screen DC
        if (hasWindow) {
            // Use PrintWindow for more reliable layered window capture, fallback to BitBlt
            BOOL ok = PrintWindow(hwnd, hMem, PW_CLIENTONLY);
            if (!ok) {
                BitBlt(hMem, 0, 0, winW, winH, hScreen, 0, 0, SRCCOPY);
            }
        } else {
            BitBlt(hMem, 0, 0, winW, winH, hScreen, wr.left, wr.top, SRCCOPY);
        }

        BITMAPINFO bmi{}; bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = winW;
        bmi.bmiHeader.biHeight = -winH; // top-down
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        std::vector<uint8_t> raw((size_t)winW * winH * 4);
        int lines = GetDIBits(hMem, hbmp, 0, winH, raw.data(), &bmi, DIB_RGB_COLORS);

        SelectObject(hMem, old);
        DeleteObject(hbmp);
        DeleteDC(hMem);
        ReleaseDC(hwnd, hScreen);

        if (lines == 0) return std::nullopt;

        Frame f;
        f.timestampUs = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();

        if (targetW == winW && targetH == winH) {
            f.width = winW; f.height = winH;
            f.data = std::move(raw);
        } else {
            // scale raw to target
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
        return f;
    }

    int m_reqW = 0, m_reqH = 0;
    bool m_useGDI = false;
    Microsoft::WRL::ComPtr<ID3D11Device> m_dev;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_ctx;
    IDXGIOutputDuplication* m_dup = nullptr;
#endif
};

// Factory used by RecordingManager
extern "C" IFrameCapture* createDesktopDuplicationCapture() {
    return new DesktopDuplication();
}
