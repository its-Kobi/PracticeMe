#include "LevelIdentity.hpp"
#include <Geode/Geode.hpp>
using namespace geode::prelude;

namespace practiceme {

template <typename T>
inline int extractInt(T& v) {
    if constexpr (requires { v.value(); }) return (int)v.value();
    else return (int)v;
}
template <typename T>
inline std::string extractString(T& v) {
    if constexpr (requires { v.c_str(); }) return std::string(v.c_str());
    else if constexpr (requires { std::string(v); }) return std::string(v);
    else return std::string(v);
}

std::string getLevelKey(GJGameLevel* level) {
    if (!level) return "unknown";
    int lid = extractInt(level->m_levelID);
    if (lid != 0) {
        return fmt::format("id_{}", lid);
    }
    std::string name = extractString(level->m_levelName);
    std::string creator = extractString(level->m_creatorName);
    int song = extractInt(level->m_songID);
    int len = extractInt(level->m_levelLength);
    std::string combined = name + "|" + creator + "|" + std::to_string(song) + "|" + std::to_string(len);
    size_t h = std::hash<std::string>{}(combined);
    return fmt::format("local_{}_{}", name, h);
}

std::string getLevelDisplayName(GJGameLevel* level) {
    if (!level) return "Unknown";
    std::string n = extractString(level->m_levelName);
    if (n.empty()) n = "Unknown";
    return n;
}

int getLevelID(GJGameLevel* level) {
    if (!level) return 0;
    return extractInt(level->m_levelID);
}

}
