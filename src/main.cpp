#include <Geode/Geode.hpp>
#include <Geode/modify/CCDirector.hpp>
#include "Settings.hpp"
#include "RecordingManager.hpp"
#include "Overlay.hpp"
#include "Hotkey.hpp"
using namespace geode::prelude;

$on_mod(Loaded){
    log::info("KRecorder loaded v{} by KOBI", Mod::get()->getVersion().toVString());
    HotkeyManager::get().init([]{
        auto& rm = RecordingManager::get();
        if(rm.isRecording()) rm.stop();
        else rm.start();
    });
}

// Director hook: polls F9 with debounce and handles frame capture
class $modify(KRecorderDirector, cocos2d::CCDirector){
    void drawScene(){
        CCDirector::drawScene();
        int vk = vkFromString(KRecorderSettings::getHotkey());
        if(HotkeyManager::get().shouldToggle(vk)){
            log::info("KRecorder: F9 detected (vk={})", vk);
            auto& rm = RecordingManager::get();
            if(rm.isRecording()) rm.stop(); else {
                if(!rm.start()){
                    log::error("KRecorder start failed: {}", rm.getLastError());
                }
            }
        }
        if(RecordingManager::get().isRecording()){
            RecordingManager::get().captureTick();
        }
    }
};

// Main menu button like Globed / Eclipse - adds KRecorder button to bottom row
#include <Geode/modify/MenuLayer.hpp>
class $modify(KRecorderMenuLayer, MenuLayer){
    bool init(){
        if(!MenuLayer::init()) return false;
        auto bottomMenu = this->getChildByID("bottom-menu");
        auto winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();
        if(bottomMenu){
            auto spr = CCSprite::create("menu_icon.png"_spr);
            if(!spr) spr = CCSprite::create("KRecorder/menu_icon.png"_spr);
            if(!spr) spr = CCSprite::createWithSpriteFrameName("GJ_recordBtn_001.png");
            if(!spr) spr = CCSprite::create("GJ_button_01.png");
            if(spr){
                spr->setScale(0.85f);
                auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(KRecorderMenuLayer::onKRecorder));
                btn->setID("krecorder-button"_spr);
                bottomMenu->addChild(btn);
                bottomMenu->updateLayout();
            }
        } else {
            auto btn = CCMenuItemSpriteExtra::create(
                ButtonSprite::create("KRecorder", 100, true, "bigFont.fnt", "GJ_button_01.png", 0, 0.8f),
                this, menu_selector(KRecorderMenuLayer::onKRecorder)
            );
            btn->setPosition({winSize.width/2, winSize.height/2 - 80});
            btn->setID("krecorder-button"_spr);
            this->addChild(btn);
        }
        log::info("KRecorder menu button added");
        return true;
    }
    void onKRecorder(CCObject*){
        geode::openSettingsPopup(Mod::get());
    }
};
