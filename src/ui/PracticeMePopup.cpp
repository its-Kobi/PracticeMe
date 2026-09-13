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
    if (ret->init(level)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool PracticeMePopup::init(GJGameLevel* level) {
    // Smart compact size: 360x270 (was 420x320) -> fits small monitors (1366x768, 800x600)
    if (!Popup::init(360.f, 270.f))
        return false;

    m_level = level;
    this->setTitle("PracticeMe");

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
        // --- COMPACT NO DATA (smart layout, small screens friendly) ---
        float cx = winSize.width / 2;
        float cy = winSize.height / 2;

        auto lvlLabel = CCLabelBMFont::create(fmt::format("\"{}\"", name).c_str(), "goldFont.fnt");
        lvlLabel->setScale(0.34f);
        lvlLabel->setPosition({cx, winSize.height - 18});
        lvlLabel->limitLabelWidth(300, 0.34f, 0.28f);
        m_mainLayer->addChild(lvlLabel);

        auto noData = CCLabelBMFont::create("NO DATA FOUND", "bigFont.fnt");
        noData->setScale(0.50f);
        noData->setColor({255, 90, 90});
        noData->setPosition({cx, cy + 38});
        m_mainLayer->addChild(noData);

        auto explain1 = CCLabelBMFont::create("PracticeMe needs some", "chatFont.fnt");
        explain1->setScale(0.50f);
        explain1->setPosition({cx, cy + 14});
        m_mainLayer->addChild(explain1);
        auto explain2 = CCLabelBMFont::create("gameplay data first.", "chatFont.fnt");
        explain2->setScale(0.50f);
        explain2->setPosition({cx, cy + 2});
        m_mainLayer->addChild(explain2);

        auto line = CCSprite::createWithSpriteFrameName("GJ_button_01.png");
        // use as thin separator (scale down heavily)
        if (line) {
            line->setScaleX(0.35f);
            line->setScaleY(0.08f);
            line->setOpacity(70);
            line->setPosition({cx, cy - 10});
            m_mainLayer->addChild(line);
        }

        auto explain3 = CCLabelBMFont::create("Play the level (incl. editor levels)", "chatFont.fnt");
        explain3->setScale(0.42f);
        explain3->setColor({200,200,200});
        explain3->setPosition({cx, cy - 22});
        m_mainLayer->addChild(explain3);
        auto explain4 = CCLabelBMFont::create("in Practice Mode to discover", "chatFont.fnt");
        explain4->setScale(0.42f);
        explain4->setColor({200,200,200});
        explain4->setPosition({cx, cy - 32});
        m_mainLayer->addChild(explain4);
        auto explain5 = CCLabelBMFont::create("weak sections.", "chatFont.fnt");
        explain5->setScale(0.42f);
        explain5->setColor({200,200,200});
        explain5->setPosition({cx, cy - 42});
        m_mainLayer->addChild(explain5);

        auto hint = CCLabelBMFont::create("(Works for editor levels too)", "chatFont.fnt");
        hint->setScale(0.38f);
        hint->setColor({130,130,130});
        hint->setPosition({cx, 18});
        m_mainLayer->addChild(hint);

        return true;
    }

    // Has data: analysis - COMPACT SMART LAYOUT
    AnalysisResult res = analyze(stats);

    float y = winSize.height - 22;

    // level name at top - compact, truncate if too long
    auto lvlLabel = CCLabelBMFont::create(fmt::format("{}  ({} atts)", name, res.totalAttempts).c_str(), "goldFont.fnt");
    lvlLabel->setScale(0.30f);
    lvlLabel->setPosition({winSize.width/2, y});
    lvlLabel->limitLabelWidth(320, 0.30f, 0.24f);
    m_mainLayer->addChild(lvlLabel);
    y -= 13;

    // OVERALL + Best on compact two-line block
    std::string overallText = fmt::format("OVERALL: {}", res.overall);
    ccColor3B overallColor = {100, 255, 100};
    if (res.overall == "WEAK") overallColor = {255, 80, 80};
    else if (res.overall == "NEEDS PRACTICE") overallColor = {255, 200, 80};
    else if (res.overall == "GOOD") overallColor = {100, 255, 100};

    auto overall = CCLabelBMFont::create(overallText.c_str(), "bigFont.fnt");
    overall->setScale(0.38f);
    overall->setColor(overallColor);
    overall->setPosition({winSize.width/2, y});
    m_mainLayer->addChild(overall);
    y -= 11;

    auto bestLabel = CCLabelBMFont::create(fmt::format("Best: {:.0f}%  |  Practice: {}", res.bestPercent, stats.totalAttemptsPractice).c_str(), "chatFont.fnt");
    bestLabel->setScale(0.42f);
    bestLabel->setColor({210,210,210});
    bestLabel->setPosition({winSize.width/2, y});
    m_mainLayer->addChild(bestLabel);
    y -= 10;

    // thin separator
    {
        auto sep = CCSprite::createWithSpriteFrameName("GJ_button_01.png");
        if (sep) {
            sep->setScaleX(0.42f);
            sep->setScaleY(0.06f);
            sep->setOpacity(60);
            sep->setPosition({winSize.width/2, y + 3});
            m_mainLayer->addChild(sep);
        }
    }
    y -= 4;

    // RECOMMENDED header
    auto recHeader = CCLabelBMFont::create("RECOMMENDED PRACTICE", "goldFont.fnt");
    recHeader->setScale(0.38f);
    recHeader->setPosition({winSize.width/2, y});
    m_mainLayer->addChild(recHeader);
    y -= 12;

    if (res.recommended.empty()) {
        auto none = CCLabelBMFont::create("No weak sections detected.", "chatFont.fnt");
        none->setScale(0.44f);
        none->setColor({150,255,150});
        none->setPosition({winSize.width/2, y});
        m_mainLayer->addChild(none);
        y -= 10;
        auto keep = CCLabelBMFont::create("Keep playing to refine.", "chatFont.fnt");
        keep->setScale(0.40f);
        keep->setColor({180,180,180});
        keep->setPosition({winSize.width/2, y});
        m_mainLayer->addChild(keep);
        y -= 12;
    } else {
        for (auto &rec : res.recommended) {
            std::string sym;
            if (rec.priority == Priority::HIGH) sym = "[!]";
            else if (rec.priority == Priority::MEDIUM) sym = "[~]";
            else sym = "[ ]";

            std::string line = fmt::format("{} {}% -> {}%  {}", sym, rec.startPercent, rec.endPercent, toString(rec.priority));
            auto lbl = CCLabelBMFont::create(line.c_str(), "bigFont.fnt");
            lbl->setScale(0.34f);
            if (rec.priority == Priority::HIGH) lbl->setColor({255, 100, 100});
            else if (rec.priority == Priority::MEDIUM) lbl->setColor({255, 220, 100});
            else lbl->setColor({160, 255, 160});
            lbl->setPosition({winSize.width/2, y});
            m_mainLayer->addChild(lbl);
            y -= 10;

            std::string detail = fmt::format("deaths: {}  atts: {}  succ: {:.0f}%", rec.deaths, rec.attempts, rec.successRate*100.f);
            auto dLbl = CCLabelBMFont::create(detail.c_str(), "chatFont.fnt");
            dLbl->setScale(0.38f);
            dLbl->setColor({190,190,190});
            dLbl->setPosition({winSize.width/2, y});
            m_mainLayer->addChild(dLbl);
            y -= 10;
            if (y < 62) break; // prevent overflow on small popup - smart clamp
        }
    }

    // WEAK SECTIONS - only if space remains
    if (res.sections.size() > res.recommended.size() && y > 78) {
        y -= 2;
        auto weakHeader = CCLabelBMFont::create("OTHER WEAK SECTIONS", "goldFont.fnt");
        weakHeader->setScale(0.30f);
        weakHeader->setColor({200,200,200});
        weakHeader->setPosition({winSize.width/2, y});
        m_mainLayer->addChild(weakHeader);
        y -= 9;
        int shown = 0;
        for (auto &s : res.sections) {
            if (shown >= 1) break; // keep compact: show max 1 extra to avoid overflow
            bool already = false;
            for (auto &r : res.recommended) if (r.startPercent==s.startPercent && r.endPercent==s.endPercent) { already=true; break; }
            if (already) continue;
            std::string line = fmt::format("{}% - {}%  {} (d:{})", s.startPercent, s.endPercent, toString(s.priority), s.deaths);
            auto lbl = CCLabelBMFont::create(line.c_str(), "chatFont.fnt");
            lbl->setScale(0.36f);
            lbl->setColor({170,170,170});
            lbl->setPosition({winSize.width/2, y});
            m_mainLayer->addChild(lbl);
            y -= 9;
            shown++;
            if (y < 62) break;
        }
    }

    // SESSION HISTORY - compact, only if space
    if (!stats.sessions.empty() && y > 58) {
        y -= 2;
        auto histHeader = CCLabelBMFont::create("SESSION HISTORY", "goldFont.fnt");
        histHeader->setScale(0.30f);
        histHeader->setColor({200,200,200});
        histHeader->setPosition({winSize.width/2, y});
        m_mainLayer->addChild(histHeader);
        y -= 9;
        int start = std::max(0, (int)stats.sessions.size() - 2); // show max 2 for compact
        for (int i=start; i<(int)stats.sessions.size(); ++i) {
            auto &sess = stats.sessions[i];
            std::string line = fmt::format("S{} - Best: {:.0f}% ({} atts)", i+1, sess.bestPercent, sess.attempts);
            auto lbl = CCLabelBMFont::create(line.c_str(), "chatFont.fnt");
            lbl->setScale(0.36f);
            lbl->setColor({160,160,160});
            lbl->setPosition({winSize.width/2, y});
            m_mainLayer->addChild(lbl);
            y -= 8;
            if (y < 48) break;
        }
    }

    // DATA QUALITY footer - always visible, compact
    std::string conf = fmt::format("DATA: {} - {}", res.confidence, res.confidenceDetail);
    if (conf.size() > 58) conf = conf.substr(0, 55) + "...";
    auto confLbl = CCLabelBMFont::create(conf.c_str(), "chatFont.fnt");
    confLbl->setScale(0.32f);
    confLbl->setColor({150,150,255});
    confLbl->setPosition({winSize.width/2, 20});
    confLbl->limitLabelWidth(320, 0.32f, 0.26f);
    m_mainLayer->addChild(confLbl);

    auto cpHint = CCLabelBMFont::create("Editor levels tracked same as normal levels", "chatFont.fnt");
    cpHint->setScale(0.30f);
    cpHint->setColor({110,110,110});
    cpHint->setPosition({winSize.width/2, 10});
    m_mainLayer->addChild(cpHint);

    return true;
}

}
