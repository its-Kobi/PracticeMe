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
std::string KRecorderSettings::getOutputDir(){ return Mod::get()->getSettingValue<std::string>("output-dir"); }
bool KRecorderSettings::isAudioEnabled(){ return Mod::get()->getSettingValue<bool>("audio-enabled"); }
