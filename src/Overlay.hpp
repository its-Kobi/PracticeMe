#pragma once
#include <Geode/Geode.hpp>
#ifdef _WIN32
#include <windows.h>
#endif
class RecordingOverlay {
public:
    static RecordingOverlay& get();
    void show();
    void hide();
    bool isVisible() const { return m_visible; }
private:
    bool m_visible=false;
    cocos2d::CCLayerColor* m_dot=nullptr;
#ifdef _WIN32
    HWND m_overlayWnd=nullptr;
    void createWinOverlay();
    void destroyWinOverlay();
#endif
};
