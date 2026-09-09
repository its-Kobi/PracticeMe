#pragma once
#include "IVideoEncoder.hpp"
// Stub for optional eclipse.ffmpeg-api integration
class FFmpegEncoder : public IVideoEncoder {
public:
    bool configure(int w,int h,int fps,int bitrate,const std::string& outPath) override { (void)w;(void)h;(void)fps;(void)bitrate;(void)outPath; return false; }
    bool encode(const Frame&) override { return false; }
    bool flush() override { return false; }
    void shutdown() override {}
    bool isHardware() const override { return false; }
};
