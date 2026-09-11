#pragma once
#include <Geode/Geode.hpp>
#include <string>
struct KRecorderSettings {
    static bool isRecordingEnabled();
    static std::string getHotkey();
    static bool isOverlayEnabled();
    static std::string getOverlayPosition();
    static int getFPS();
    static int getWidth();
    static int getHeight();
    static int getBitrate();
    static std::string getCodec();
    static std::string getFormat();
    static std::string getOutputDir();
    static bool isAudioEnabled();
    static std::string getAudioMode();
    static std::string getMicDevice();
};
