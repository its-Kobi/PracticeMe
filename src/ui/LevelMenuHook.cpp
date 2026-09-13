#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/EditLevelLayer.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>
#include "PracticeMePopup.hpp"

using namespace geode::prelude;
#include <Geode/binding/LevelInfoLayer.hpp>
#include <Geode/binding/EditLevelLayer.hpp>
#include <Geode/binding/EditorPauseLayer.hpp>
#include <Geode/binding/LevelEditorLayer.hpp>

namespace {
    CCMenuItemSpriteExtra* createPracticeMeBtn(CCObject* target, SEL_MenuHandler cb) {
        CCSprite* spr = CCSprite::create("level_menu_icon.png"_spr);
        if (!spr) spr = CCSprite::createWithSpriteFrameName("GJ_practiceBtn_001.png");
        CCMenuItemSpriteExtra* btn = nullptr;
        if (spr) {
            // Smart fit: original icon 64x64, scale 0.60 => ~38px, matches LevelInfoLayer icons (~42px)
            spr->setScale(0.60f);
            // Ensure content size matches scaled sprite (Geode menus use content size for layout)
            btn = CCMenuItemSpriteExtra::create(spr, target, cb);
            // Tighter hitbox
            btn->setContentSize(spr->getScaledContentSize());
        } else {
            auto btnSpr = ButtonSprite::create("PracticeMe", 80, true, "bigFont.fnt", "GJ_button_01.png", 30, 0.6f);
            btn = CCMenuItemSpriteExtra::create(btnSpr, target, cb);
        }
        btn->setID("practiceme-button"_spr);
        return btn;
    }
}

class $modify(PracticeMeLevelInfoLayer, LevelInfoLayer) {
    bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge)) return false;

        auto btn = createPracticeMeBtn(this, menu_selector(PracticeMeLevelInfoLayer::onPracticeMe));

        if (auto leftMenu = this->getChildByID("left-side-menu")) {
            leftMenu->addChild(btn);
            leftMenu->updateLayout();
        } else if (auto menu = this->getChildByID("other-menu")) {
            menu->addChild(btn);
            menu->updateLayout();
        } else {
            auto fallbackMenu = CCMenu::create();
            fallbackMenu->setID("practiceme-menu"_spr);
            fallbackMenu->setPosition({60, 60});
            fallbackMenu->addChild(btn);
            this->addChild(fallbackMenu);
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

class $modify(PracticeMeEditLevelLayer, EditLevelLayer) {
    bool init(GJGameLevel* level) {
        if (!EditLevelLayer::init(level)) return false;

        auto btn = createPracticeMeBtn(this, menu_selector(PracticeMeEditLevelLayer::onPracticeMe));

        // Smart placement: try side actions menu (vertical), fallback to level-edit-menu (center bottom)
        if (auto actionsMenu = this->getChildByID("level-actions-menu")) {
            // Insert near top of vertical list but keep layout gaps intact
            actionsMenu->addChild(btn);
            actionsMenu->updateLayout();
        } else if (auto editMenu = this->getChildByID("level-edit-menu")) {
            editMenu->addChild(btn);
            editMenu->updateLayout();
        } else {
            // Last resort: own menu bottom-left
            auto fallback = CCMenu::create();
            fallback->setID("practiceme-menu"_spr);
            fallback->setPosition({60, 60});
            fallback->addChild(btn);
            this->addChild(fallback);
        }

        log::info("[PracticeMe] Button added to EditLevelLayer for {}", level ? level->m_levelName : "unknown");
        return true;
    }

    void onPracticeMe(CCObject* sender) {
        auto level = m_level;
        if (!level) {
            log::warn("[PracticeMe] No level in EditLevelLayer");
            return;
        }
        practiceme::PracticeMePopup::create(level)->show();
    }
};

class $modify(PracticeMeEditorPause, EditorPauseLayer) {
    bool init(LevelEditorLayer* editor) {
        if (!EditorPauseLayer::init(editor)) return false;

        // Level may be editor's current level
        auto btn = createPracticeMeBtn(this, menu_selector(PracticeMeEditorPause::onPracticeMe));

        // Prefer adding to a visible small menu; EditorPauseLayer has many detached menus
        // Try small-actions-menu first, then actions-menu, then resume-menu, then fallback
        if (auto m = this->getChildByID("small-actions-menu")) {
            m->addChild(btn);
            m->updateLayout();
        } else if (auto m = this->getChildByID("actions-menu")) {
            m->addChild(btn);
            m->updateLayout();
        } else if (auto m = this->getChildByID("resume-menu")) {
            m->addChild(btn);
            m->updateLayout();
        } else {
            auto fallback = CCMenu::create();
            fallback->setID("practiceme-menu"_spr);
            // place top-right near pause bg, but safe for small screens
            auto winSize = CCDirector::sharedDirector()->getWinSize();
            fallback->setPosition({winSize.width - 60, 60});
            fallback->addChild(btn);
            this->addChild(fallback);
        }

        GJGameLevel* lvl = nullptr;
        if (editor) lvl = editor->m_level;
        log::info("[PracticeMe] Button added to EditorPauseLayer for {}", lvl ? lvl->m_levelName : "editor");
        return true;
    }

    void onPracticeMe(CCObject* sender) {
        GJGameLevel* lvl = nullptr;
        if (this->m_editorLayer) lvl = this->m_editorLayer->m_level;
        if (!lvl) {
            log::warn("[PracticeMe] No level in EditorPauseLayer");
            return;
        }
        practiceme::PracticeMePopup::create(lvl)->show();
    }
};
