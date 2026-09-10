#include "Overlay.hpp"
#include "Settings.hpp"
using namespace geode::prelude;
RecordingOverlay& RecordingOverlay::get(){ static RecordingOverlay i; return i; }

#ifdef _WIN32
void RecordingOverlay::createWinOverlay(){
    if(m_overlayWnd) return;
    auto hwndGD = FindWindowA(nullptr, "Geometry Dash");
    if(!hwndGD) hwndGD = FindWindowA("GLFW30", nullptr);
    if(!hwndGD) return;
    RECT rc{}; GetWindowRect(hwndGD, &rc);
    RECT cr{}; GetClientRect(hwndGD, &cr);
    POINT pt{0,0}; ClientToScreen(hwndGD, &pt);
    int winW = cr.right; int winH = cr.bottom;
    float sz=10.f; float m=12.f;
    std::string pos = KRecorderSettings::getOverlayPosition();
    int x = pt.x + winW - (int)(m + sz/2);
    int y = pt.y + (int)(m + sz/2);
    if(pos=="top-left"){ x = pt.x + (int)m; y = pt.y + (int)m; }
    else if(pos=="bottom-left"){ x = pt.x + (int)m; y = pt.y + winH - (int)(m+sz); }
    else if(pos=="bottom-right"){ x = pt.x + winW - (int)(m+sz); y = pt.y + winH - (int)(m+sz); }
    else { x = pt.x + winW - (int)(m+sz); y = pt.y + (int)m; } // top-right default

    WNDCLASSA wc{}; wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = GetModuleHandleA(nullptr);
    wc.lpszClassName = "KRecorderOverlayCls";
    RegisterClassA(&wc);
    m_overlayWnd = CreateWindowExA(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        "KRecorderOverlayCls", nullptr, WS_POPUP,
        x, y, (int)sz, (int)sz, nullptr, nullptr, wc.hInstance, nullptr);
    if(m_overlayWnd){
        SetLayeredWindowAttributes(m_overlayWnd, 0, 255, LWA_ALPHA);
        HBRUSH br = CreateSolidBrush(RGB(255,0,0));
        HDC hdc = GetDC(m_overlayWnd);
        RECT r{0,0,(int)sz,(int)sz};
        FillRect(hdc, &r, br);
        DeleteObject(br);
        ReleaseDC(m_overlayWnd, hdc);
        ShowWindow(m_overlayWnd, SW_SHOWNA);
        UpdateWindow(m_overlayWnd);
    }
}
void RecordingOverlay::destroyWinOverlay(){
    if(m_overlayWnd){ DestroyWindow(m_overlayWnd); m_overlayWnd=nullptr; }
}
#endif

void RecordingOverlay::show(){
    if(m_visible) return;
    if(!KRecorderSettings::isOverlayEnabled()) return;
#ifdef _WIN32
    // Use separate window so it is NOT captured by window DC BitBlt (game-only)
    createWinOverlay();
    m_visible = m_overlayWnd != nullptr;
    return;
#else
    auto dir = cocos2d::CCDirector::sharedDirector();
    auto scene = dir->getRunningScene();
    if(!scene) return;
    if(m_dot){ m_dot->setVisible(true); m_visible=true; return; }
    auto win = dir->getWinSize();
    float sz=10.f; float m=12.f;
    std::string pos = KRecorderSettings::getOverlayPosition();
    cocos2d::CCPoint p;
    if(pos=="top-left") p = cocos2d::CCPoint(m+sz/2, win.height - m - sz/2);
    else if(pos=="bottom-left") p = cocos2d::CCPoint(m+sz/2, m+sz/2);
    else if(pos=="bottom-right") p = cocos2d::CCPoint(win.width - m - sz/2, m+sz/2);
    else p = cocos2d::CCPoint(win.width - m - sz/2, win.height - m - sz/2);
    m_dot = cocos2d::CCLayerColor::create(cocos2d::ccc4(255,0,0,255), sz, sz);
    m_dot->setPosition(p - ccp(sz/2,sz/2));
    m_dot->setZOrder(9999);
    m_dot->setID("krecorder-indicator");
    scene->addChild(m_dot);
    m_visible=true;
#endif
}
void RecordingOverlay::hide(){
    if(!m_visible) return;
#ifdef _WIN32
    destroyWinOverlay();
    m_visible=false;
#else
    if(m_dot) m_dot->setVisible(false);
    m_visible=false;
#endif
}
