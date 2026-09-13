#include "PracticeMePopup.hpp"
#include "../data/Storage.hpp"
#include "../data/LevelStats.hpp"
#include "../analysis/SectionAnalysis.hpp"
#include "../util/LevelIdentity.hpp"
#include <Geode/Geode.hpp>
#include <Geode/binding/GJGameLevel.hpp>

using namespace geode::prelude;

namespace practiceme {

PracticeMePopup* PracticeMePopup::create(GJGameLevel* level) {
    auto ret = new PracticeMePopup();
    if (ret->initAnchored(420.f, 320.f, level)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool PracticeMePopup::setup(GJGameLevel* level) {
    m_level = level;
    this->setTitle("PracticeMe");
    // title already set

    auto winSize = m_mainLayer->getContentSize();

    Storage::get().load();
    std::string key = getLevelKey(level);
    std::string name = getLevelDisplayName(level);
    LevelStats stats;
    bool hasData = false;
    if (auto opt = Storage::get().get(key)) {
        stats = opt->get();
        if (stats.totalAttemptsPractice > 0) hasData = true;
    }

    if (!hasData) {
        // NO DATA FOUND
        auto noData = CCLabelBMFont::create("NO DATA FOUND", "bigFont.fnt");
        noData->setScale(0.6f);
        noData->setColor({255, 80, 80});
        noData->setPosition({winSize.width/2, winSize.height/2 + 60});
        m_mainLayer->addChild(noData);

        auto explain1 = CCLabelBMFont::create("PracticeMe needs some", "chatFont.fnt");
        explain1->setScale(0.6f);
        explain1->setPosition({winSize.width/2, winSize.height/2 + 22});
        m_mainLayer->addChild(explain1);
        auto explain2 = CCLabelBMFont::create("gameplay data first.", "chatFont.fnt");
        explain2->setScale(0.6f);
        explain2->setPosition({winSize.width/2, winSize.height/2 + 6});
        m_mainLayer->addChild(explain2);
        auto explain3 = CCLabelBMFont::create("Play the level for a while", "chatFont.fnt");
        explain3->setScale(0.55f);
        explain3->setPosition({winSize.width/2, winSize.height/2 - 14});
        m_mainLayer->addChild(explain3);
        auto explain4 = CCLabelBMFont::create("so PracticeMe can discover", "chatFont.fnt");
        explain4->setScale(0.55f);
        explain4->setPosition({winSize.width/2, winSize.height/2 - 28});
        m_mainLayer->addChild(explain4);
        auto explain5 = CCLabelBMFont::create("which parts need more practice.", "chatFont.fnt");
        explain5->setScale(0.55f);
        explain5->setPosition({winSize.width/2, winSize.height/2 - 42});
        m_mainLayer->addChild(explain5);

        // level name
        auto lvlLabel = CCLabelBMFont::create(fmt::format("\"{}\"", name).c_str(), "goldFont.fnt");
        lvlLabel->setScale(0.4f);
        lvlLabel->setPosition({winSize.width/2, winSize.height/2 + 88});
        m_mainLayer->addChild(lvlLabel);

        // Show close hint
        auto hint = CCLabelBMFont::create("(Play in Practice Mode for best results)", "chatFont.fnt");
        hint->setScale(0.45f);
        hint->setColor({150,150,150});
        hint->setPosition({winSize.width/2, 30});
        m_mainLayer->addChild(hint);

        return true;
    }

    // Has data: analysis
    AnalysisResult res = analyze(stats);

    float y = winSize.height - 32;

    // level name at top
    auto lvlLabel = CCLabelBMFont::create(fmt::format("{}  ({} attempts)", name, res.totalAttempts).c_str(), "goldFont.fnt");
    lvlLabel->setScale(0.35f);
    lvlLabel->setPosition({winSize.width/2, y});
    m_mainLayer->addChild(lvlLabel);
    y -= 16;

    // OVERALL
    std::string overallText = fmt::format("OVERALL: {}", res.overall);
    ccColor3B overallColor = {100, 255, 100};
    if (res.overall == "WEAK") overallColor = {255, 80, 80};
    else if (res.overall == "NEEDS PRACTICE") overallColor = {255, 200, 80};
    else if (res.overall == "GOOD") overallColor = {100, 255, 100};

    auto overall = CCLabelBMFont::create(overallText.c_str(), "bigFont.fnt");
    overall->setScale(0.45f);
    overall->setColor(overallColor);
    overall->setPosition({winSize.width/2, y});
    m_mainLayer->addChild(overall);
    y -= 14;

    auto bestLabel = CCLabelBMFont::create(fmt::format("Best: {:.0f}%   Practice attempts: {}", res.bestPercent, stats.totalAttemptsPractice).c_str(), "chatFont.fnt");
    bestLabel->setScale(0.5f);
    bestLabel->setPosition({winSize.width/2, y});
    m_mainLayer->addChild(bestLabel);
    y -= 14;

    // separator
    auto sep = CCSprite::create("GJ_button_01.png");
    // use simple line via colored sprite
    // Instead draw not needed, just spacing
    y -= 2;

    // RECOMMENDED PRACTICE header
    auto recHeader = CCLabelBMFont::create("RECOMMENDED PRACTICE", "goldFont.fnt");
    recHeader->setScale(0.45f);
    recHeader->setPosition({winSize.width/2, y});
    m_mainLayer->addChild(recHeader);
    y -= 14;

    if (res.recommended.empty()) {
        auto none = CCLabelBMFont::create("No specific weak sections detected.", "chatFont.fnt");
        none->setScale(0.5f);
        none->setColor({150,255,150});
        none->setPosition({winSize.width/2, y});
        m_mainLayer->addChild(none);
        y -= 12;
        auto keep = CCLabelBMFont::create("Keep playing to refine analysis.", "chatFont.fnt");
        keep->setScale(0.45f);
        keep->setPosition({winSize.width/2, y});
        m_mainLayer->addChild(keep);
        y -= 14;
    } else {
        for (auto &rec : res.recommended) {
            std::string icon = " ";
            if (rec.priority == Priority::HIGH) icon = "🔥";
            else if (rec.priority == Priority::MEDIUM) icon = "⚠";
            else if (rec.priority == Priority::LOW) icon = "✓";
            // For BMFont we can't render emoji reliably, use text symbols
            std::string sym;
            if (rec.priority == Priority::HIGH) sym = "[!]";
            else if (rec.priority == Priority::MEDIUM) sym = "[~]";
            else sym = "[ ]";

            std::string line = fmt::format("{} {}% -> {}%  {}", sym, rec.startPercent, rec.endPercent, toString(rec.priority));
            auto lbl = CCLabelBMFont::create(line.c_str(), "bigFont.fnt");
            lbl->setScale(0.38f);
            if (rec.priority == Priority::HIGH) lbl->setColor({255, 100, 100});
            else if (rec.priority == Priority::MEDIUM) lbl->setColor({255, 220, 100});
            else lbl->setColor({160, 255, 160});
            lbl->setPosition({winSize.width/2, y});
            m_mainLayer->addChild(lbl);
            y -= 12;

            std::string detail = fmt::format("deaths: {}  atts: {}  succ: {:.0f}%", rec.deaths, rec.attempts, rec.successRate*100.f);
            auto dLbl = CCLabelBMFont::create(detail.c_str(), "chatFont.fnt");
            dLbl->setScale(0.45f);
            dLbl->setColor({200,200,200});
            dLbl->setPosition({winSize.width/2, y});
            m_mainLayer->addChild(dLbl);
            y -= 12;
            if (y < 75) break; // avoid overflow
        }
    }

    // WEAK SECTIONS (if more than recommended)
    if (res.sections.size() > res.recommended.size() && y > 90) {
        y -= 4;
        auto weakHeader = CCLabelBMFont::create("WEAK SECTIONS", "goldFont.fnt");
        weakHeader->setScale(0.35f);
        weakHeader->setPosition({winSize.width/2, y});
        m_mainLayer->addChild(weakHeader);
        y -= 10;
        int shown = 0;
        for (auto &s : res.sections) {
            if (shown >= 2) break;
            bool already = false;
            for (auto &r : res.recommended) if (r.startPercent==s.startPercent && r.endPercent==s.endPercent) { already=true; break; }
            if (already) continue;
            std::string line = fmt::format("{}% - {}%  {} (d:{})", s.startPercent, s.endPercent, toString(s.priority), s.deaths);
            auto lbl = CCLabelBMFont::create(line.c_str(), "chatFont.fnt");
            lbl->setScale(0.42f);
            lbl->setPosition({winSize.width/2, y});
            m_mainLayer->addChild(lbl);
            y -= 9;
            shown++;
            if (y < 75) break;
        }
    }

    // SESSION HISTORY compact
    if (!stats.sessions.empty() && y > 70) {
        y -= 4;
        auto histHeader = CCLabelBMFont::create("SESSION HISTORY", "goldFont.fnt");
        histHeader->setScale(0.35f);
        histHeader->setPosition({winSize.width/2, y});
        m_mainLayer->addChild(histHeader);
        y -= 10;
        int start = std::max(0, (int)stats.sessions.size() - 4);
        for (int i=start; i<(int)stats.sessions.size(); ++i) {
            auto &sess = stats.sessions[i];
            std::string line = fmt::format("Session {} - Best: {:.0f}% ({} atts)", i+1, sess.bestPercent, sess.attempts);
            auto lbl = CCLabelBMFont::create(line.c_str(), "chatFont.fnt");
            lbl->setScale(0.42f);
            lbl->setPosition({winSize.width/2, y});
            m_mainLayer->addChild(lbl);
            y -= 9;
            if (y < 55) break;
        }
    }

    // DATA QUALITY footer
    std::string conf = fmt::format("DATA QUALITY: {} - {}", res.confidence, res.confidenceDetail);
    // truncate if too long
    if (conf.size() > 65) conf = conf.substr(0, 62) + "...";
    auto confLbl = CCLabelBMFont::create(conf.c_str(), "chatFont.fnt");
    confLbl->setScale(0.38f);
    confLbl->setColor({150,150,255});
    confLbl->setPosition({winSize.width/2, 26});
    m_mainLayer->addChild(confLbl);

    // checkpoint info hint
    auto cpHint = CCLabelBMFont::create(fmt::format("Checkpoints tracked: practice starts recorded").c_str(), "chatFont.fnt");
    cpHint->setScale(0.35f);
    cpHint->setColor({120,120,120});
    cpHint->setPosition({winSize.width/2, 16});
    m_mainLayer->addChild(cpHint);

    return true;
}

}
