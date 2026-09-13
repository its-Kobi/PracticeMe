#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "TrackingManager.hpp"

using namespace geode::prelude;

class $modify(PracticeMePlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        practiceme::TrackingManager::get().onPlayLayerEnter(level, this);
        // initial attempt will be started via resetLevel
        return true;
    }

    void onQuit() {
        practiceme::TrackingManager::get().onPlayLayerExit();
        PlayLayer::onQuit();
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        // after reset, new attempt starts
        bool isPractice = this->m_isPracticeMode;
        // update current percent tracking for start position
        float pct = this->getCurrentPercent();
        practiceme::TrackingManager::get().onPercentUpdate(pct);
        practiceme::TrackingManager::get().onAttemptStart(isPractice);
    }

    void levelComplete() {
        float pct = this->getCurrentPercent();
        if (pct < 99.f) pct = 100.f;
        practiceme::TrackingManager::get().onComplete(pct, this->m_isPracticeMode);
        PlayLayer::levelComplete();
    }

    void destroyPlayer(PlayerObject* player, GameObject* obj) {
        // capture percent at death; call before original destroys
        if (player == m_player1 || player == m_player2) {
            float pct = this->getCurrentPercent();
            // if percent is 0 at very start, still record
            practiceme::TrackingManager::get().onDeath(pct, this->m_isPracticeMode);
        }
        PlayLayer::destroyPlayer(player, obj);
    }

    void updateVisibility(float dt) {
        PlayLayer::updateVisibility(dt);
        // lightweight per-frame percent polling but not heavy: only when in level
        // throttle to avoid expensive? It's cheap (getCurrentPercent is simple)
        // Only update every ~0.2s to save
        static float s_acc = 0;
        s_acc += dt;
        if (s_acc > 0.15f) {
            s_acc = 0;
            float pct = this->getCurrentPercent();
            practiceme::TrackingManager::get().onPercentUpdate(pct);
        }
    }
};
