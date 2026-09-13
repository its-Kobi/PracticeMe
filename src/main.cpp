#include <Geode/Geode.hpp>
#include "data/Storage.hpp"

using namespace geode::prelude;

$on_mod(Loaded) {
    log::info("[PracticeMe] Loaded v{} by KOBI", Mod::get()->getVersion().toVString());
    practiceme::Storage::get().load();
}

$on_mod(DataLoaded) {
    // ensure storage loaded after save dir ready
    practiceme::Storage::get().load();
}
