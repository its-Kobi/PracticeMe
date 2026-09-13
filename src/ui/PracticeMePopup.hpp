#pragma once
#include <Geode/Geode.hpp>

class GJGameLevel;

namespace practiceme {
class PracticeMePopup : public geode::Popup {
protected:
    GJGameLevel* m_level = nullptr;
public:
    static PracticeMePopup* create(GJGameLevel* level);
    bool init(GJGameLevel* level);
};
}
