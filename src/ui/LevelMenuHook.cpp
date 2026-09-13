#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include "PracticeMePopup.hpp"

using namespace geode::prelude;
#include <Geode/binding/LevelInfoLayer.hpp>

class $modify(PracticeMeLevelInfoLayer, LevelInfoLayer) {
    bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge)) return false;

        // Add PracticeMe button to left menu or create
        // LevelInfoLayer has play button etc. Add to levelInfo layer menu.
        // Find existing menu or create button near bottom.

        // Try to find left side menu or bottom menu
        auto winSize = CCDirector::sharedDirector()->getWinSize();

        // Create sprite button
        // Use custom sprite if exists else fallback to ButtonSprite
        CCSprite* spr = nullptr;
        // try resources: level_menu_icon.png
        spr = CCSprite::create("level_menu_icon.png"_spr);
        if (!spr) spr = CCSprite::createWithSpriteFrameName("GJ_practiceBtn_001.png");

        ButtonSprite* btnSpr = nullptr;
        CCMenuItemSpriteExtra* btn = nullptr;
        if (spr) {
            spr->setScale(0.85f);
            btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(PracticeMeLevelInfoLayer::onPracticeMe));
        } else {
            btnSpr = ButtonSprite::create("PracticeMe", 90, true, "bigFont.fnt", "GJ_button_01.png", 30, 0.7f);
            btn = CCMenuItemSpriteExtra::create(btnSpr, this, menu_selector(PracticeMeLevelInfoLayer::onPracticeMe));
        }
        btn->setID("practiceme-button"_spr);

        // Try to place inside left menu
        if (auto leftMenu = this->getChildByID("left-side-menu")) {
            leftMenu->addChild(btn);
            leftMenu->updateLayout();
        } else if (auto menu = this->getChildByID("other-menu")) {
            menu->addChild(btn);
            menu->updateLayout();
        } else {
            // fallback: create own menu at bottom left
            auto menu = CCMenu::create();
            menu->setID("practiceme-menu"_spr);
            menu->setPosition({60, 60});
            menu->addChild(btn);
            this->addChild(menu);
        }

        log::info("[PracticeMe] Button added to LevelInfoLayer for {}", level ? level->m_levelName : "unknown");
        return true;
    }

    void onPracticeMe(CCObject* sender) {
        auto level = m_level;
        if (!level) {
            log::warn("[PracticeMe] No level in LevelInfoLayer");
            return;
        }
        practiceme::PracticeMePopup::create(level)->show();
    }
};
