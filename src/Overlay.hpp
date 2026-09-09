#pragma once
#include <Geode/Geode.hpp>
class RecordingOverlay {
public:
    static RecordingOverlay& get();
    void show();
    void hide();
    bool isVisible() const { return m_visible; }
private:
    bool m_visible=false;
    cocos2d::CCLayerColor* m_dot=nullptr;
};
