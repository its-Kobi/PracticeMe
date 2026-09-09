#include "WMFEncoder.hpp"
#include "FFmpegEncoder.hpp"
#include "IVideoEncoder.hpp"
#include <memory>
std::unique_ptr<IVideoEncoder> createEncoder(const std::string& pref){
    if(pref=="h264"||pref=="auto"||pref=="hevc"){
        auto e = std::make_unique<WMFEncoder>();
        return e;
    }
    return std::make_unique<FFmpegEncoder>();
}
std::unique_ptr<IVideoEncoder> createEncoderForCurrentSettings();
