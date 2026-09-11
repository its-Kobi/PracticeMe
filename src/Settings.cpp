#include "Settings.hpp"
using namespace geode::prelude;
bool KRecorderSettings::isRecordingEnabled(){ return Mod::get()->getSettingValue<bool>("recording-enabled"); }
std::string KRecorderSettings::getHotkey(){ return Mod::get()->getSettingValue<std::string>("recording-hotkey"); }
bool KRecorderSettings::isOverlayEnabled(){ return Mod::get()->getSettingValue<bool>("overlay-enabled"); }
std::string KRecorderSettings::getOverlayPosition(){ return Mod::get()->getSettingValue<std::string>("overlay-position"); }
int KRecorderSettings::getFPS(){ return (int)Mod::get()->getSettingValue<int64_t>("video-fps"); }
int KRecorderSettings::getWidth(){ return (int)Mod::get()->getSettingValue<int64_t>("video-width"); }
int KRecorderSettings::getHeight(){ return (int)Mod::get()->getSettingValue<int64_t>("video-height"); }
int KRecorderSettings::getBitrate(){ return (int)Mod::get()->getSettingValue<int64_t>("video-bitrate"); }
std::string KRecorderSettings::getCodec(){ return Mod::get()->getSettingValue<std::string>("video-codec"); }
std::string KRecorderSettings::getFormat(){
    try { return Mod::get()->getSettingValue<std::string>("video-format"); } catch(...) { return "mp4"; }
}
std::string KRecorderSettings::getOutputDir(){ return Mod::get()->getSettingValue<std::string>("output-dir"); }
bool KRecorderSettings::isAudioEnabled(){ return Mod::get()->getSettingValue<bool>("audio-enabled"); }
std::string KRecorderSettings::getAudioMode(){
    try {
        auto m = Mod::get()->getSettingValue<std::string>("audio-mode");
        if(m=="disabled"||m=="game"||m=="mic"||m=="game+mic") return m;
    } catch(...) {}
    // Fallback to legacy bool
    try { if(Mod::get()->getSettingValue<bool>("audio-enabled")) return "game"; } catch(...) {}
    return "game";
}
std::string KRecorderSettings::getMicDevice(){ try { return Mod::get()->getSettingValue<std::string>("audio-mic-device"); } catch(...) { return ""; } }
