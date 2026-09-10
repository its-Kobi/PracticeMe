#pragma once
#include "IVideoEncoder.hpp"
#ifdef _WIN32
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wrl/client.h>
#endif
class WMFEncoder : public IVideoEncoder {
public:
    bool configure(int w,int h,int fps,int bitrate,const std::string& outPath) override;
    bool encode(const Frame& f) override;
    bool flush() override;
    void shutdown() override;
    bool isHardware() const override { return m_hardware; }
private:
    bool m_hardware=false;
    bool m_inited=false;
    int m_width=0, m_height=0;
    int64_t m_startTimeUs=-1;
    int64_t m_lastTimeUs=0;
#ifdef _WIN32
    Microsoft::WRL::ComPtr<IMFSinkWriter> m_writer;
    DWORD m_stream=0;
    LONGLONG m_frameIdx=0;
    int m_fps=60;
#endif
};
