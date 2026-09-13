#pragma once
#include <string>
#include <vector>
#include <array>
#include <matjson.hpp>

namespace practiceme {

enum class Priority {
    GOOD,
    LOW,
    MEDIUM,
    HIGH
};

inline const char* toString(Priority p) {
    switch(p) {
        case Priority::HIGH: return "HIGH PRIORITY";
        case Priority::MEDIUM: return "MEDIUM PRIORITY";
        case Priority::LOW: return "LOW PRIORITY";
        case Priority::GOOD: return "GOOD";
    }
    return "GOOD";
}

struct SessionRecord {
    int64_t timestamp = 0;
    int attempts = 0;
    float bestPercent = 0.f;
    float startPercent = 0.f;
    float endPercent = 0.f;
    bool practiceMode = true;

    matjson::Value toJson() const;
    static SessionRecord fromJson(const matjson::Value& v);
};

struct LevelStats {
    static constexpr int kVersion = 1;
    static constexpr int kBuckets = 101;

    std::string key;          // unique level identifier
    std::string levelName = "Unknown";
    int levelID = 0;

    int totalAttemptsPractice = 0;
    int totalAttemptsNormal = 0;
    float bestPractice = 0.f;
    float bestNormal = 0.f;

    int sessionCount = 0;

    // per-percent buckets 0..100 inclusive
    std::array<int, kBuckets> deathsPractice{};
    std::array<int, kBuckets> deathsNormal{};
    std::array<int, kBuckets> passesPractice{};
    std::array<int, kBuckets> passesNormal{};
    std::array<int, kBuckets> startsPractice{};
    std::array<int, kBuckets> startsNormal{};

    std::vector<SessionRecord> sessions;

    matjson::Value toJson() const;
    static LevelStats fromJson(const matjson::Value& v);

    int totalDeathsPractice() const;
    int totalAttempts() const { return totalAttemptsPractice + totalAttemptsNormal; }
};

// analysis output
struct WeakSection {
    int startPercent = 0;
    int endPercent = 0;
    Priority priority = Priority::GOOD;
    int deaths = 0;
    int attempts = 0;
    float successRate = 1.f;
    float score = 0.f;
};

struct Recommendation {
    int startPercent = 0;
    int endPercent = 0;
    Priority priority = Priority::GOOD;
    int deaths = 0;
    int attempts = 0;
    float bestInRange = 0.f;
    float successRate = 1.f;
};

struct AnalysisResult {
    std::vector<WeakSection> sections;
    std::vector<Recommendation> recommended;
    std::string overall = "GOOD"; // GOOD / NEEDS PRACTICE / WEAK
    std::string confidence = "NO DATA"; // NO DATA / LOW DATA / MEDIUM DATA / HIGH CONFIDENCE
    std::string confidenceDetail = "";
    int totalAttempts = 0;
    float bestPercent = 0.f;
};

}
