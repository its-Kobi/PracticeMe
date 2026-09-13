#pragma once
#include <string>
#include <optional>
class GJGameLevel;
class PlayLayer;

namespace practiceme {

class TrackingManager {
public:
    static TrackingManager& get();

    void onPlayLayerEnter(GJGameLevel* level, PlayLayer* pl);
    void onPlayLayerExit();
    // Called on resetLevel: new attempt started
    void onAttemptStart(bool isPracticeMode);
    // Called when player dies
    void onDeath(float percent, bool isPracticeMode);
    // Called when level completed
    void onComplete(float percent, bool isPracticeMode);
    // Update best percent polling or checkpoint? Call occasionally
    void onPercentUpdate(float percent);

    std::string currentKey() const { return m_currentKey; }
    GJGameLevel* currentLevel() const { return m_currentLevel; }

    // session bookkeeping
    void beginSession(bool isPracticeMode);
    void endSession();

private:
    TrackingManager() = default;

    std::string m_currentKey;
    GJGameLevel* m_currentLevel = nullptr;
    PlayLayer* m_playLayer = nullptr;

    bool m_inLevel = false;
    bool m_isPracticeMode = false;

    // attempt tracking
    float m_attemptStartPercent = 0.f;
    float m_bestInAttempt = 0.f;
    bool m_hasAttempt = false;
    int m_attemptCountThisSession = 0;
    float m_sessionBest = 0.f;
    float m_currentPercent = 0.f;

    int64_t m_sessionStartTime = 0;

    float clampPercent(float p) const;
    void recordAttempt(float endPercent, bool died, bool completed);
};

}
