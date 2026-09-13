#include "SectionAnalysis.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace practiceme {

// Lightweight deterministic analysis
// Steps:
// 1. Build per-percent score based on death density + fail rate + start frequency adjustment
// 2. Smooth scores with 3-point moving average
// 3. Cluster contiguous percents above threshold into sections
// 4. Classify priority
// 5. Generate recommendations sorted by priority/score
// 6. Determine overall + confidence

AnalysisResult analyze(const LevelStats& stats) {
    AnalysisResult res;
    res.totalAttempts = stats.totalAttemptsPractice;
    res.bestPercent = stats.bestPractice;

    int totalPractice = stats.totalAttemptsPractice;
    int totalDeaths = 0;
    for (int d : stats.deathsPractice) totalDeaths += d;

    // confidence logic
    if (totalPractice == 0) {
        res.confidence = "NO DATA";
        res.confidenceDetail = "Not enough practice attempts.";
        res.overall = "NO DATA";
        return res;
    }
    if (totalPractice < 3) {
        res.confidence = "LOW DATA";
        res.confidenceDetail = "Only a few attempts — preliminary.";
    } else if (totalPractice < 10) {
        res.confidence = "PRELIMINARY ANALYSIS";
        res.confidenceDetail = "More attempts will improve accuracy.";
    } else if (totalPractice < 25) {
        res.confidence = "MEDIUM DATA";
        res.confidenceDetail = "Decent data — recommendations are becoming reliable.";
    } else {
        res.confidence = "HIGH CONFIDENCE";
        res.confidenceDetail = "Strong data — recommendations are reliable.";
    }
    if (totalPractice >= 5 && totalPractice < 10 && totalDeaths < 3) {
        res.confidence = "LOW DATA";
        res.confidenceDetail = "Too few deaths to be confident.";
    }

    // If very few deaths, we still give analysis but low priority
    // Build per-percent arrays
    constexpr int N = LevelStats::kBuckets; // 101
    std::array<float, N> rawScore{};
    std::array<float, N> failRate{};
    std::array<int, N> attemptsAt{};

    // compute attemptsAt per bucket: passes + deaths
    int maxDeaths = 1;
    for (int i = 0; i < N; ++i) maxDeaths = std::max(maxDeaths, stats.deathsPractice[i]);

    for (int i = 0; i < N; ++i) {
        int deaths = stats.deathsPractice[i];
        int passes = stats.passesPractice[i];
        int attempts = deaths + passes;
        attemptsAt[i] = attempts;
        if (attempts < 2) {
            failRate[i] = 0.f;
        } else {
            failRate[i] = (float)deaths / (float)attempts;
        }
        // normalized death
        float normDeath = (float)deaths / (float)maxDeaths;
        // score: weighted combo
        // 0.6 * normDeath + 0.5 * failRate  (failRate already 0-1)
        // if attempts < 2, reduce weight of failRate
        float w = attempts < 3 ? 0.2f : 1.f;
        rawScore[i] = 0.65f * normDeath + 0.55f * failRate[i] * w;
        // Slight boost if many starts around here but low success = checkpoint struggle
        // If starts high and failRate high, increase score
        // starts normalized
        // we don't have max starts easily, but use heuristic
        if (stats.startsPractice[i] > 2 && failRate[i] > 0.5f) {
            rawScore[i] += 0.15f;
        }
        if (rawScore[i] > 1.f) rawScore[i] = 1.f;
    }

    // smoothing: 5-point window (i-2..i+2)
    std::array<float, N> smooth{};
    for (int i = 0; i < N; ++i) {
        float sum = 0; int cnt = 0;
        for (int d = -2; d <= 2; ++d) {
            int j = i + d;
            if (j < 0 || j >= N) continue;
            float weight = (d == 0) ? 1.f : (std::abs(d) == 1 ? 0.7f : 0.35f);
            sum += rawScore[j] * weight;
            cnt++;
        }
        // normalize by weights sum approx
        // compute weight sum for existing j
        float wsum = 0;
        for (int d = -2; d <= 2; ++d) { int j=i+d; if(j<0||j>=N) continue; wsum += (d==0?1.f:(std::abs(d)==1?0.7f:0.35f)); }
        smooth[i] = sum / wsum;
    }

    // Cluster into sections where smooth > thresholds
    // thresholds for clustering: >0.05 is considered weak; we will merge
    struct TmpSec { int s,e; float maxScore; float avgScore; int deaths; int atts; };
    std::vector<TmpSec> tmp;
    bool in = false;
    int curS = 0;
    for (int i = 0; i < N; ++i) {
        bool isWeak = smooth[i] > 0.08f && attemptsAt[i] >= 1;
        // also require at least 1 death in neighborhood
        if (isWeak) {
            bool hasDeathNear = false;
            for (int d = -2; d <=2; ++d){int j=i+d; if(j<0||j>=N) continue; if(stats.deathsPractice[j]>0) hasDeathNear=true;}
            if (!hasDeathNear) isWeak = false;
        }
        if (isWeak && !in) { in=true; curS=i; }
        else if (!isWeak && in) {
            // end section at i-1
            int curE = i-1;
            // extend to include trailing deaths within 2
            // merge small gaps later
            TmpSec sec; sec.s = curS; sec.e = curE;
            // compute metrics
            float m = 0, sum=0; int dsum=0, asum=0;
            for(int k=curS;k<=curE;++k){ m=std::max(m, smooth[k]); sum+=smooth[k]; dsum+=stats.deathsPractice[k]; asum+=attemptsAt[k]; }
            sec.maxScore=m; sec.avgScore=sum/(curE-curS+1); sec.deaths=dsum; sec.atts=asum;
            // filter tiny sections with 1 death and low score
            if (sec.deaths >=1 && sec.avgScore > 0.09f) tmp.push_back(sec);
            in=false;
        }
    }
    if (in) {
        int curE = N-1;
        TmpSec sec; sec.s=curS; sec.e=curE;
        float m=0,sum=0;int dsum=0,asum=0;
        for(int k=curS;k<=curE;++k){ m=std::max(m,smooth[k]); sum+=smooth[k]; dsum+=stats.deathsPractice[k]; asum+=attemptsAt[k]; }
        sec.maxScore=m; sec.avgScore=sum/(curE-curS+1); sec.deaths=dsum; sec.atts=asum;
        if (sec.deaths>=1 && sec.avgScore>0.09f) tmp.push_back(sec);
    }

    // merge gaps <3% between sections
    std::vector<TmpSec> merged;
    for (auto &s : tmp) {
        if (merged.empty()) merged.push_back(s);
        else {
            auto &prev = merged.back();
            int gap = s.s - prev.e - 1;
            if (gap <= 3) {
                // merge
                prev.e = s.e;
                prev.maxScore = std::max(prev.maxScore, s.maxScore);
                // recompute avg/deaths/atts
                float sum=0; int dsum=0,asum=0;
                for(int k=prev.s;k<=prev.e;++k){ sum+=smooth[k]; dsum+=stats.deathsPractice[k]; asum+=attemptsAt[k]; }
                prev.avgScore = sum/(prev.e-prev.s+1);
                prev.deaths = dsum;
                prev.atts = asum;
            } else {
                merged.push_back(s);
            }
        }
    }

    // Now classify each into WeakSection with priority
    std::vector<WeakSection> sections;
    for (auto &m : merged) {
        WeakSection ws;
        ws.startPercent = m.s;
        ws.endPercent = m.e;
        // expand slightly for practice recommendation: give 2% before
        // but keep as detected
        ws.deaths = m.deaths;
        ws.attempts = m.atts;
        ws.score = m.avgScore;
        ws.successRate = m.atts > 0 ? 1.f - (float)m.deaths / (float)m.atts : 1.f;
        if (ws.successRate < 0) ws.successRate = 0;
        if (ws.successRate > 1) ws.successRate = 1;

        // Insufficient data guard
        if (m.atts < 3) {
            // not enough attempts to claim difficulty
            // downgrade to LOW or keep but mark low confidence
            if (m.avgScore < 0.25f) ws.priority = Priority::LOW;
            else ws.priority = Priority::MEDIUM;
        } else {
            // thresholds based on avgScore and fail
            float avgFail = 0;
            int cnt=0;
            for(int k=m.s;k<=m.e;++k){ if(attemptsAt[k]>=2) { avgFail+=failRate[k]; cnt++; } }
            if(cnt>0) avgFail/=cnt;

            if (m.avgScore >= 0.35f || avgFail >= 0.65f) ws.priority = Priority::HIGH;
            else if (m.avgScore >= 0.20f || avgFail >= 0.4f) ws.priority = Priority::MEDIUM;
            else if (m.avgScore >= 0.12f) ws.priority = Priority::LOW;
            else ws.priority = Priority::GOOD;
        }

        // If deaths==0 shouldn't be here, but safeguard
        if (ws.deaths == 0) ws.priority = Priority::GOOD;

        // Additional adaptive: if successRate high (>0.85) but score medium, downgrade
        if (ws.successRate > 0.85f && ws.priority == Priority::MEDIUM) ws.priority = Priority::LOW;
        if (ws.successRate > 0.92f && ws.priority != Priority::HIGH) ws.priority = Priority::GOOD;

        // Never claim HIGH with only 1-2 attempts
        if (ws.attempts < 4 && ws.priority == Priority::HIGH) ws.priority = Priority::MEDIUM;

        sections.push_back(ws);
    }

    // Sort by priority then score descending
    std::sort(sections.begin(), sections.end(), [](const WeakSection& a, const WeakSection& b){
        if (a.priority != b.priority) return (int)a.priority > (int)b.priority; // HIGH first (3>2...)
        return a.score > b.score;
    });

    // Determine overall
    if (sections.empty()) {
        // No weak sections found
        if (totalPractice < 3) res.overall = "NEEDS PRACTICE";
        else if (stats.bestPractice >= 95.f) res.overall = "GOOD";
        else res.overall = "GOOD";
    } else {
        int high = 0, med=0;
        for(auto& s: sections) if(s.priority==Priority::HIGH) high++; else if(s.priority==Priority::MEDIUM) med++;
        if (high >= 2) res.overall = "WEAK";
        else if (high == 1) res.overall = "NEEDS PRACTICE";
        else if (med >= 2) res.overall = "NEEDS PRACTICE";
        else res.overall = "GOOD";
    }

    res.sections = sections;

    // Build recommendations: top sections, but ensure up to 3, expand range slightly for practice
    for (auto &ws : sections) {
        if (res.recommended.size() >= 3) break;
        if (ws.priority == Priority::GOOD) continue;
        Recommendation r;
        r.startPercent = std::max(0, ws.startPercent - 2);
        r.endPercent = std::min(100, ws.endPercent + 3);
        // ensure at least 5% width for practical practice
        if (r.endPercent - r.startPercent < 5) r.endPercent = std::min(100, r.startPercent + 8);
        r.priority = ws.priority;
        r.deaths = ws.deaths;
        r.attempts = ws.attempts;
        r.successRate = ws.successRate;
        // bestInRange: approximate as max passed within? Use bestPractice if within range else 0
        r.bestInRange = stats.bestPractice; // simplified
        res.recommended.push_back(r);
    }

    // If no recommendations but still weak? fallback to most died percent
    if (res.recommended.empty() && !sections.empty()) {
        // pick highest score even if GOOD? no
    }
    // If still no recommendations and totalPractice >=3 but no sections, give a generic "keep practicing to 100"
    if (res.recommended.empty() && totalPractice >= 3 && stats.bestPractice < 100) {
        // find max death percent
        int maxD = 0, maxIdx=0;
        for(int i=0;i<N;++i) if(stats.deathsPractice[i]>maxD){maxD=stats.deathsPractice[i]; maxIdx=i;}
        if (maxD>0) {
            Recommendation r;
            r.startPercent = std::max(0, maxIdx-3);
            r.endPercent = std::min(100, maxIdx+5);
            r.priority = Priority::LOW;
            r.deaths = maxD;
            r.attempts = attemptsAt[maxIdx];
            r.successRate = r.attempts ? 1.f - (float)r.deaths/r.attempts : 1.f;
            r.bestInRange = stats.bestPractice;
            res.recommended.push_back(r);
        }
    }

    return res;
}

}
