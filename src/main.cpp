#include <Geode/Geode.hpp>
#include <Geode/modify/CCDirector.hpp>
#include <Geode/cocos/include/cocos2d.h>
#ifdef _WIN32
#include <windows.h>
#include <GL/gl.h>
#pragma comment(lib,"opengl32.lib")
#endif
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

// Director hook: polls hotkey and does GAME-ONLY GL capture (no desktop, no block)
class $modify(KRecorderDirector, cocos2d::CCDirector){
    void drawScene(){
        // Hotkey poll before draw so F9 is responsive
        int vk = vkFromString(KRecorderSettings::getHotkey());
        if(HotkeyManager::get().shouldToggle(vk)){
            log::info("KRecorder: {} detected (vk={})", KRecorderSettings::getHotkey(), vk);
            auto& rm = RecordingManager::get();
            if(rm.isRecording()) rm.stop(); else {
                if(!rm.start()){
                    log::error("KRecorder start failed: {}", rm.getLastError());
                }
            }
        }

        CCDirector::drawScene();

        // Game-only capture: read from GL front buffer after swap (game frame, not desktop)
        // This is the correct source: GD's OpenGL FBO, never desktop/taskbar
        if(RecordingManager::get().isRecording()){
#ifdef _WIN32
            auto director = CCDirector::sharedDirector();
            if(director && director->getOpenGLView()){
                auto frameSize = director->getOpenGLView()->getFrameSize();
                int w = (int)frameSize.width;
                int h = (int)frameSize.height;
                // Fallback to winSize if frameSize is 0
                if(w <= 0 || h <= 0){
                    auto winSize = director->getWinSize();
                    w = (int)winSize.width; h = (int)winSize.height;
                }
                int reqW = RecordingManager::get().getWidth();
                int reqH = RecordingManager::get().getHeight();
                if(reqW > 0) w = reqW;
                if(reqH > 0) h = reqH;
                if(w%2) w--; if(h%2) h--;
                if(w < 64 || h < 64) {
                    auto winSize = director->getWinSize();
                    w = (int)winSize.width; h = (int)winSize.height;
                }

                auto t0 = std::chrono::steady_clock::now();
                std::vector<uint8_t> data((size_t)w * h * 4);
                // GD uses OpenGL, front buffer now contains just-rendered frame
                glReadBuffer(GL_FRONT);
                glReadPixels(0, 0, w, h, GL_BGRA, GL_UNSIGNED_BYTE, data.data());
                auto t1 = std::chrono::steady_clock::now();
                // Flip vertically (GL origin bottom-left, encoder expects top-left)
                std::vector<uint8_t> flipped((size_t)w * h * 4);
                for(int y=0; y<h; ++y){
                    memcpy(flipped.data() + (size_t)y * w * 4, data.data() + (size_t)(h-1-y) * w * 4, (size_t)w * 4);
                }
                Frame f; f.width = w; f.height = h; f.data = std::move(flipped);
                f.timestampUs = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
                auto t2 = std::chrono::steady_clock::now();
                // Push to async queue - never blocks game thread (drops if encoder behind)
                RecordingManager::get().onFrame(f);
                auto t3 = std::chrono::steady_clock::now();
                static int s_cnt=0; static auto s_last=std::chrono::steady_clock::now();
                s_cnt++;
                double capMs = std::chrono::duration_cast<std::chrono::microseconds>(t1-t0).count()/1000.0;
                double flipMs = std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count()/1000.0;
                double totalMs = std::chrono::duration_cast<std::chrono::microseconds>(t3-t0).count()/1000.0;
                if(totalMs > 8.0 || s_cnt % 120 == 0){
                    geode::log::info("KRecorder GL capture {}x{} cap={:.1f}ms flip={:.1f} total={:.1f}ms", w, h, capMs, flipMs, totalMs);
                }
                if(totalMs > 15.0) geode::log::warn("KRecorder: slow GL capture {:.1f}ms", totalMs);
            } else {
                // Fallback to GDI window capture (should not happen)
                RecordingManager::get().captureTick();
            }
#else
            RecordingManager::get().captureTick();
#endif
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
                spr->setScale(0.65f);
                spr->setRotation(-90.f);
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
