#include "Storage.hpp"
#include <Geode/loader/Mod.hpp>
#include <Geode/utils/file.hpp>
#include <Geode/utils/general.hpp>
#include <matjson.hpp>
#include <Geode/Geode.hpp>

using namespace geode::prelude;

namespace practiceme {

Storage& Storage::get() {
    static Storage inst;
    return inst;
}

std::string Storage::filePath() const {
    auto dir = Mod::get()->getSaveDir();
    return (dir / "practiceme.json").string();
}

void Storage::load() {
    if (m_loaded) return;
    m_loaded = true;
    auto path = filePath();
    auto res = utils::file::readString(path);
    if (!res.isOk()) {
        log::info("[PracticeMe] No existing data file, starting fresh");
        return;
    }
    auto content = res.unwrap();
    if (content.empty()) return;
    auto parsed = matjson::parse(content);
    if (!parsed) {
        log::warn("[PracticeMe] Failed to parse save file: {}", parsed.unwrapErr());
        return;
    }
    auto val = parsed.unwrap();
    if (!val.isObject()) {
        log::warn("[PracticeMe] Save file not an object");
        return;
    }
    // format: { "levels": { key: LevelStatsJson }, "ver": 1 }  or legacy flat map
    matjson::Value levelsVal;
    if (val.contains("levels")) levelsVal = val["levels"];
    else levelsVal = val;

    if (!levelsVal.isObject()) return;
    for (auto& [k, v] : levelsVal) {
        try {
            auto stats = LevelStats::fromJson(v);
            stats.key = k;
            // clamp sanity
            if (stats.totalAttemptsPractice < 0) stats.totalAttemptsPractice = 0;
            if (stats.totalAttemptsNormal < 0) stats.totalAttemptsNormal = 0;
            if (stats.bestPractice < 0) stats.bestPractice = 0;
            if (stats.bestPractice > 100) stats.bestPractice = 100;
            if (stats.bestNormal < 0) stats.bestNormal = 0;
            if (stats.bestNormal > 100) stats.bestNormal = 100;
            m_data[k] = std::move(stats);
        } catch (...) {
            log::warn("[PracticeMe] Skipping corrupted entry {}", k);
        }
    }
    log::info("[PracticeMe] Loaded {} levels", m_data.size());
}

void Storage::save() {
    // build json
    auto obj = matjson::Value::object();
    for (auto& [k, stats] : m_data) {
        obj[k] = stats.toJson();
    }
    auto root = matjson::Value::object();
    root["ver"] = 1;
    root["levels"] = obj;

    std::string out = root.dump(matjson::NO_INDENTATION);
    auto path = filePath();
    auto dir = Mod::get()->getSaveDir().string();
    // ensure dir exists via Geode file utils: write will create? ensure parent
    (void)utils::file::writeString(path, out);
    m_dirty = false;
    log::info("[PracticeMe] Saved {} levels to {}", m_data.size(), path);
}

LevelStats& Storage::getOrCreate(const std::string& key, const std::string& name, int levelID) {
    auto it = m_data.find(key);
    if (it != m_data.end()) {
        // update name if changed
        if (!name.empty() && name != "Unknown") it->second.levelName = name;
        if (levelID != 0) it->second.levelID = levelID;
        return it->second;
    }
    LevelStats s;
    s.key = key;
    s.levelName = name;
    s.levelID = levelID;
    m_data[key] = std::move(s);
    return m_data[key];
}

std::optional<std::reference_wrapper<LevelStats>> Storage::get(const std::string& key) {
    auto it = m_data.find(key);
    if (it == m_data.end()) return std::nullopt;
    return std::ref(it->second);
}

}
