#pragma once
#include <Geode/Geode.hpp>

class GJGameLevel;

namespace practiceme {
class PracticeMePopup : public geode::Popup<GJGameLevel*> {
protected:
    bool setup(GJGameLevel* level) override;
    GJGameLevel* m_level = nullptr;
public:
    static PracticeMePopup* create(GJGameLevel* level);
};
}
