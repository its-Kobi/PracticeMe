#include "Overlay.hpp"
#include "Settings.hpp"
using namespace geode::prelude;
RecordingOverlay& RecordingOverlay::get(){ static RecordingOverlay i; return i; }
void RecordingOverlay::show(){
    if(m_visible) return;
    if(!KRecorderSettings::isOverlayEnabled()) return;
    auto dir = cocos2d::CCDirector::sharedDirector();
    auto scene = dir->getRunningScene();
    if(!scene) return;
    auto win = dir->getWinSize();
    float sz=10.f; float m=12.f;
    std::string pos = KRecorderSettings::getOverlayPosition();
    cocos2d::CCPoint p;
    if(pos=="top-left") p={m+sz/2, win.height - m - sz/2};
    else if(pos=="bottom-left") p={m+sz/2, m+sz/2};
    else if(pos=="bottom-right") p={win.width - m - sz/2, m+sz/2};
    else p={win.width - m - sz/2, win.height - m - sz/2};
    m_dot = cocos2d::CCLayerColor::create(cocos2d::ccc4(255,0,0,255), sz, sz);
    // circle via shader not needed, small rect with rounded via sprite would be nicer
    m_dot->setPosition(p - ccp(sz/2,sz/2));
    m_dot->setZOrder(9999);
    m_dot->setID("krecorder-indicator");
    scene->addChild(m_dot);
    m_visible=true;
}
void RecordingOverlay::hide(){
    if(!m_visible) return;
    if(m_dot){ m_dot->removeFromParent(); m_dot=nullptr; }
    m_visible=false;
}
