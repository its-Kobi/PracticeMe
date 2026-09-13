#include "LevelStats.hpp"
#include <chrono>

namespace practiceme {

matjson::Value SessionRecord::toJson() const {
    auto v = matjson::Value::object();
    v["t"] = (double)timestamp;
    v["a"] = attempts;
    v["b"] = bestPercent;
    v["sp"] = startPercent;
    v["ep"] = endPercent;
    v["pm"] = practiceMode;
    return v;
}

SessionRecord SessionRecord::fromJson(const matjson::Value& v) {
    SessionRecord s;
    try {
        if (v.contains("t")) s.timestamp = (int64_t)v["t"].asDouble().unwrapOr(0);
        if (v.contains("a")) s.attempts = (int)v["a"].asInt().unwrapOr(0);
        if (v.contains("b")) s.bestPercent = (float)v["b"].asDouble().unwrapOr(0);
        if (v.contains("sp")) s.startPercent = (float)v["sp"].asDouble().unwrapOr(0);
        if (v.contains("ep")) s.endPercent = (float)v["ep"].asDouble().unwrapOr(0);
        if (v.contains("pm")) s.practiceMode = v["pm"].asBool().unwrapOr(true);
    } catch(...) {}
    return s;
}

matjson::Value LevelStats::toJson() const {
    auto v = matjson::Value::object();
    v["ver"] = kVersion;
    v["key"] = key;
    v["name"] = levelName;
    v["lid"] = levelID;
    v["tap"] = totalAttemptsPractice;
    v["tan"] = totalAttemptsNormal;
    v["bp"] = bestPractice;
    v["bn"] = bestNormal;
    v["sc"] = sessionCount;

    auto arrToJson = [](const std::array<int,kBuckets>& arr) {
        auto a = matjson::Value::array();
        for (int x : arr) a.push(x);
        return a;
    };
    v["dp"] = arrToJson(deathsPractice);
    v["dn"] = arrToJson(deathsNormal);
    v["pp"] = arrToJson(passesPractice);
    v["pn"] = arrToJson(passesNormal);
    v["sp"] = arrToJson(startsPractice);
    v["sn"] = arrToJson(startsNormal);

    auto sess = matjson::Value::array();
    // keep last 20 sessions only in storage to stay lightweight
    size_t start = sessions.size() > 20 ? sessions.size() - 20 : 0;
    for (size_t i = start; i < sessions.size(); ++i) sess.push(sessions[i].toJson());
    v["sess"] = sess;
    return v;
}

LevelStats LevelStats::fromJson(const matjson::Value& v) {
    LevelStats s;
    try {
        if (v.contains("key")) s.key = v["key"].asString().unwrapOr("unknown");
        if (v.contains("name")) s.levelName = v["name"].asString().unwrapOr("Unknown");
        if (v.contains("lid")) s.levelID = (int)v["lid"].asInt().unwrapOr(0);
        if (v.contains("tap")) s.totalAttemptsPractice = (int)v["tap"].asInt().unwrapOr(0);
        if (v.contains("tan")) s.totalAttemptsNormal = (int)v["tan"].asInt().unwrapOr(0);
        if (v.contains("bp")) s.bestPractice = (float)v["bp"].asDouble().unwrapOr(0);
        if (v.contains("bn")) s.bestNormal = (float)v["bn"].asDouble().unwrapOr(0);
        if (v.contains("sc")) s.sessionCount = (int)v["sc"].asInt().unwrapOr(0);

        auto arrFrom = [&](const char* key, std::array<int,kBuckets>& arr){
            if (!v.contains(key)) return;
            auto& a = v[key];
            if (!a.isArray()) return;
            auto vec = a.asArray().unwrapOr(std::vector<matjson::Value>{});
            for (size_t i = 0; i < vec.size() && i < (size_t)kBuckets; ++i) {
                arr[i] = (int)vec[i].asInt().unwrapOr(0);
                if (arr[i] < 0) arr[i] = 0;
                if (arr[i] > 100000) arr[i] = 100000;
            }
        };
        arrFrom("dp", s.deathsPractice);
        arrFrom("dn", s.deathsNormal);
        arrFrom("pp", s.passesPractice);
        arrFrom("pn", s.passesNormal);
        arrFrom("sp", s.startsPractice);
        arrFrom("sn", s.startsNormal);

        if (v.contains("sess") && v["sess"].isArray()) {
            auto vec = v["sess"].asArray().unwrap();
            for (auto& e : vec) s.sessions.push_back(SessionRecord::fromJson(e));
        }
    } catch(...) {}
    return s;
}

int LevelStats::totalDeathsPractice() const {
    int sum = 0;
    for (int x : deathsPractice) sum += x;
    return sum;
}

}
