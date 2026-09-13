#include "TrackingManager.hpp"
#include "../data/Storage.hpp"
#include "../data/LevelStats.hpp"
#include "../util/LevelIdentity.hpp"
#include <Geode/Geode.hpp>
#include <chrono>

using namespace geode::prelude;

namespace practiceme {

TrackingManager& TrackingManager::get() {
    static TrackingManager inst;
    return inst;
}

float TrackingManager::clampPercent(float p) const {
    if (p < 0) return 0;
    if (p > 100) return 100;
    return p;
}

void TrackingManager::onPlayLayerEnter(GJGameLevel* level, PlayLayer* pl) {
    Storage::get().load();
    if (!level) return;
    auto key = getLevelKey(level);
    auto name = getLevelDisplayName(level);
    int lid = getLevelID(level);
    m_currentKey = key;
    m_currentLevel = level;
    m_playLayer = pl;
    m_inLevel = true;
    m_hasAttempt = false;
    m_attemptStartPercent = 0.f;
    m_currentPercent = 0.f;
    m_bestInAttempt = 0.f;
    m_attemptCountThisSession = 0;
    m_sessionBest = 0.f;
    // ensure entry exists
    Storage::get().getOrCreate(key, name, lid);
    log::info("[PracticeMe] Enter level {} ({})", name, key);
    // detect practice mode at enter
    if (pl) {
        // PlayLayer::m_isPracticeMode
        m_isPracticeMode = pl->m_isPracticeMode;
    }
    m_sessionStartTime = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    beginSession(m_isPracticeMode);
}

void TrackingManager::onPlayLayerExit() {
    if (!m_inLevel) return;
    // finalize session if needed
    endSession();
    // Save
    Storage::get().save();
    log::info("[PracticeMe] Exit level {}", m_currentKey);
    m_inLevel = false;
    m_playLayer = nullptr;
    m_currentLevel = nullptr;
    m_currentKey.clear();
    m_hasAttempt = false;
}

void TrackingManager::beginSession(bool isPracticeMode) {
    m_isPracticeMode = isPracticeMode;
    m_attemptCountThisSession = 0;
    m_sessionBest = 0.f;
    m_sessionStartTime = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void TrackingManager::endSession() {
    if (m_attemptCountThisSession == 0) return;
    if (m_currentKey.empty()) return;
    auto opt = Storage::get().get(m_currentKey);
    if (!opt) return;
    LevelStats& stats = opt->get();
    SessionRecord rec;
    rec.timestamp = m_sessionStartTime;
    rec.attempts = m_attemptCountThisSession;
    rec.bestPercent = m_sessionBest;
    rec.practiceMode = m_isPracticeMode;
    // keep last 50 sessions trimming done in toJson but also limit here
    stats.sessions.push_back(rec);
    if (stats.sessions.size() > 50) stats.sessions.erase(stats.sessions.begin(), stats.sessions.begin() + (stats.sessions.size() - 50));
    stats.sessionCount++;
    Storage::get().markDirty();
    // save periodically
    Storage::get().save();
    log::info("[PracticeMe] Session saved: {} attempts best {}% practice={}", rec.attempts, rec.bestPercent, rec.practiceMode);
    m_attemptCountThisSession = 0;
    m_sessionBest = 0.f;
}

void TrackingManager::onAttemptStart(bool isPracticeMode) {
    if (!m_inLevel) return;
    // if we had a previous attempt without death/complete, it was a restart; count as attempt with no death? ignore
    // update practice mode if changed
    m_isPracticeMode = isPracticeMode;
    m_attemptStartPercent = clampPercent(m_currentPercent);
    // In practice mode, the start percent may be checkpoint position. m_currentPercent at reset reflects checkpoint.
    // For normal mode, start is 0.
    if (!isPracticeMode) m_attemptStartPercent = 0.f;
    m_bestInAttempt = m_attemptStartPercent;
    m_hasAttempt = true;
    // record start bucket
    if (m_currentKey.empty()) return;
    auto opt = Storage::get().get(m_currentKey);
    if (!opt) return;
    LevelStats& stats = opt->get();
    int bucket = (int)std::round(m_attemptStartPercent);
    if (bucket < 0) bucket = 0;
    if (bucket > 100) bucket = 100;
    if (isPracticeMode) stats.startsPractice[bucket] += 1;
    else stats.startsNormal[bucket] += 1;
    if (isPracticeMode) stats.totalAttemptsPractice += 1;
    else stats.totalAttemptsNormal += 1;
    m_attemptCountThisSession++;
    log::info("[PracticeMe] Attempt start {}% practice={}", m_attemptStartPercent, isPracticeMode);
}

void TrackingManager::onPercentUpdate(float percent) {
    percent = clampPercent(percent);
    m_currentPercent = percent;
    if (percent > m_bestInAttempt) m_bestInAttempt = percent;
    if (percent > m_sessionBest) m_sessionBest = percent;
    // update best in LevelStats
    if (!m_currentKey.empty()) {
        auto opt = Storage::get().get(m_currentKey);
        if (opt) {
            LevelStats& stats = opt->get();
            if (m_isPracticeMode) {
                if (percent > stats.bestPractice) stats.bestPractice = percent;
            } else {
                if (percent > stats.bestNormal) stats.bestNormal = percent;
            }
        }
    }
}

void TrackingManager::recordAttempt(float endPercent, bool died, bool completed) {
    if (!m_hasAttempt) return;
    if (m_currentKey.empty()) return;
    auto opt = Storage::get().get(m_currentKey);
    if (!opt) return;
    LevelStats& stats = opt->get();

    endPercent = clampPercent(endPercent);
    float start = clampPercent(m_attemptStartPercent);
    if (endPercent < start) endPercent = start; // handle weird backwards progress

    // passes: for each integer percent between start and end-1 (if died) or start..100 (if completed), increment passes
    // dies: increment death bucket
    int s = (int)std::floor(start);
    int e = (int)std::floor(endPercent);
    if (s < 0) s = 0; if (s > 100) s = 100;
    if (e < 0) e = 0; if (e > 100) e = 100;

    if (died) {
        // passes for s .. e-1
        for (int i = s; i < e; ++i) {
            if (m_isPracticeMode) stats.passesPractice[i] += 1;
            else stats.passesNormal[i] += 1;
        }
        // death at e
        if (m_isPracticeMode) stats.deathsPractice[e] += 1;
        else stats.deathsNormal[e] += 1;
    } else if (completed) {
        for (int i = s; i <= 100; ++i) {
            if (i <= 100) {
                if (m_isPracticeMode) stats.passesPractice[std::min(i,100)] += 1;
                else stats.passesNormal[std::min(i,100)] += 1;
            }
        }
        // no death
    } else {
        // exited without death? treat as passes up to best
        for (int i = s; i < e; ++i) {
            if (m_isPracticeMode) stats.passesPractice[i] += 1;
            else stats.passesNormal[i] += 1;
        }
    }

    // update best
    if (endPercent > m_sessionBest) m_sessionBest = endPercent;

    Storage::get().markDirty();
    // debounced save: save immediately but lightweight
    Storage::get().save();
    m_hasAttempt = false;
    log::info("[PracticeMe] Record attempt start {}% -> end {}% died={} completed={}", start, endPercent, died, completed);
}

void TrackingManager::onDeath(float percent, bool isPracticeMode) {
    if (!m_inLevel) return;
    percent = clampPercent(percent);
    onPercentUpdate(percent);
    recordAttempt(percent, true, false);
    log::info("[PracticeMe] Death at {}% practice={}", percent, isPracticeMode);
}

void TrackingManager::onComplete(float percent, bool isPracticeMode) {
    if (!m_inLevel) return;
    percent = clampPercent(percent);
    onPercentUpdate(percent);
    recordAttempt(percent, false, true);
    log::info("[PracticeMe] Complete {}% practice={}", percent, isPracticeMode);
}

}
