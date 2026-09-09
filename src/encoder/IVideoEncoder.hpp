#pragma once
#include "../capture/IFrameCapture.hpp"
#include <string>
class IVideoEncoder {
public:
    virtual ~IVideoEncoder() = default;
    virtual bool configure(int w,int h,int fps,int bitrate,const std::string& outPath) = 0;
    virtual bool encode(const Frame& f) = 0;
    virtual bool flush() = 0;
    virtual void shutdown() = 0;
    virtual bool isHardware() const = 0;
};
