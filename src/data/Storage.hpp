#pragma once
#include "LevelStats.hpp"
#include <string>
#include <unordered_map>
#include <optional>
#include <mutex>

namespace practiceme {

class Storage {
public:
    static Storage& get();

    void load();
    void save();

    LevelStats& getOrCreate(const std::string& key, const std::string& name = "Unknown", int levelID = 0);
    std::optional<std::reference_wrapper<LevelStats>> get(const std::string& key);
    const std::unordered_map<std::string, LevelStats>& all() const { return m_data; }

    void markDirty() { m_dirty = true; }

private:
    Storage() = default;
    std::string filePath() const;
    std::unordered_map<std::string, LevelStats> m_data;
    bool m_dirty = false;
    bool m_loaded = false;
    std::mutex m_mutex;
};

}
