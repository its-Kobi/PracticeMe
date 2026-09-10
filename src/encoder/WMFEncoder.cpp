#include "WMFEncoder.hpp"
#include <Geode/Geode.hpp>
#ifdef _WIN32
#include <codecapi.h>
#include <windows.h>
#pragma comment(lib,"mfplat.lib")
#pragma comment(lib,"mfreadwrite.lib")
#pragma comment(lib,"mfuuid.lib")

static std::wstring toWidePath(const std::string& s) {
    if (s.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (len <= 0) {
        // fallback ANSI
        len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, nullptr, 0);
        std::wstring w(len, L'\0');
        MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, w.data(), len);
        if (!w.empty() && w.back() == L'\0') w.pop_back();
        return w;
    }
    std::wstring w(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), len);
    if (!w.empty() && w.back() == L'\0') w.pop_back();
    return w;
}
#endif

bool WMFEncoder::configure(int w,int h,int fps,int bitrate,const std::string& outPath){
#ifdef _WIN32
    m_width = w; m_height = h; m_fps = fps;
    HRESULT hr = MFStartup(MF_VERSION);
    if(FAILED(hr)){ geode::log::error("KRecorder: MFStartup failed {}", (int)hr); return false; }
    Microsoft::WRL::ComPtr<IMFAttributes> attrs;
    MFCreateAttributes(&attrs, 1);
    attrs->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
    std::wstring wpath = toWidePath(outPath);
    hr = MFCreateSinkWriterFromURL(wpath.c_str(), nullptr, attrs.Get(), &m_writer);
    if(FAILED(hr)){
        geode::log::info("KRecorder: hardware MFT failed {}, trying software", (int)hr);
        attrs->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, FALSE);
        hr = MFCreateSinkWriterFromURL(wpath.c_str(), nullptr, attrs.Get(), &m_writer);
        m_hardware=false;
    } else m_hardware=true;
    if(FAILED(hr)){
        geode::log::error("KRecorder: MFCreateSinkWriterFromURL failed {} path={}", (int)hr, outPath);
        return false;
    }
    geode::log::info("KRecorder: Encoder initialized {} {}x{} @{}fps bitrate={}kbps hw={}", outPath, w, h, fps, bitrate, m_hardware);

    Microsoft::WRL::ComPtr<IMFMediaType> outType, inType;
    MFCreateMediaType(&outType);
    outType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    outType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
    outType->SetUINT32(MF_MT_AVG_BITRATE, bitrate*1000);
    outType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    MFSetAttributeSize(outType.Get(), MF_MT_FRAME_SIZE, w, h);
    MFSetAttributeRatio(outType.Get(), MF_MT_FRAME_RATE, fps, 1);
    MFSetAttributeRatio(outType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1,1);
    // Force baseline profile for compatibility
    // outType->SetUINT32(MF_MT_MPEG2_PROFILE, eAVEncH264VProfile_Base);

    HRESULT hrAdd = m_writer->AddStream(outType.Get(), &m_stream);
    if(FAILED(hrAdd)){ geode::log::error("AddStream failed {}", (int)hrAdd); return false; }

    MFCreateMediaType(&inType);
    inType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    inType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    MFSetAttributeSize(inType.Get(), MF_MT_FRAME_SIZE, w, h);
    MFSetAttributeRatio(inType.Get(), MF_MT_FRAME_RATE, fps, 1);
    MFSetAttributeRatio(inType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1,1);
    // Top-down RGB32: negative stride would be bottom-up, we use top-down (positive)
    inType->SetUINT32(MF_MT_DEFAULT_STRIDE, w * 4);
    // Ensure progressive
    inType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);

    hr = m_writer->SetInputMediaType(m_stream, inType.Get(), nullptr);
    if(FAILED(hr)){ geode::log::error("SetInputMediaType failed {}", (int)hr); return false; }

    hr = m_writer->BeginWriting();
    if(FAILED(hr)){ geode::log::error("BeginWriting failed {}", (int)hr); return false; }
    m_inited=true;
    m_frameIdx=0;
    m_startTimeUs=-1;
    m_lastTimeUs=0;
    geode::log::info("KRecorder: Encoder BeginWriting OK");
    return true;
#else
    (void)w;(void)h;(void)fps;(void)bitrate;(void)outPath; return false;
#endif
}
bool WMFEncoder::encode(const Frame& f){
#ifdef _WIN32
    if(!m_inited || !m_writer) return false;
    // Validate size - if Frame size differs from encoder config, drop or log
    if(f.width != m_width || f.height != m_height){
        geode::log::warn("KRecorder: frame size mismatch {}x{} vs encoder {}x{} - dropping", f.width, f.height, m_width, m_height);
        return false;
    }
    if(f.data.size() != (size_t)f.width * f.height * 4){
        geode::log::warn("KRecorder: frame data size mismatch {}", f.data.size());
        return false;
    }
    Microsoft::WRL::ComPtr<IMFMediaBuffer> buf;
    DWORD len = (DWORD)f.data.size();
    HRESULT hr = MFCreateMemoryBuffer(len, &buf);
    if(FAILED(hr)) return false;
    BYTE* ptr=nullptr; DWORD maxLen=0,curLen=0;
    hr = buf->Lock(&ptr,&maxLen,&curLen);
    if(FAILED(hr)) return false;
    memcpy(ptr, f.data.data(), len);
    buf->Unlock();
    buf->SetCurrentLength(len);
    Microsoft::WRL::ComPtr<IMFSample> sample;
    MFCreateSample(&sample);
    sample->AddBuffer(buf.Get());
    // Use real capture timestamps for correct playback speed (fixes speeded video at 30/60 fps)
    if(m_startTimeUs < 0) m_startTimeUs = (int64_t)f.timestampUs;
    int64_t ptsUs = (int64_t)f.timestampUs - m_startTimeUs;
    LONGLONG pts100ns = ptsUs * 10; // us -> 100ns
    LONGLONG duration100ns = 10000000LL / m_fps;
    if(m_lastTimeUs > 0 && f.timestampUs > (uint64_t)m_lastTimeUs){
        duration100ns = ((int64_t)f.timestampUs - m_lastTimeUs) * 10;
        if(duration100ns <= 0) duration100ns = 10000000LL / m_fps;
        if(duration100ns > 10000000LL * 2) duration100ns = 10000000LL / m_fps; // clamp huge gaps
    }
    m_lastTimeUs = (int64_t)f.timestampUs;
    sample->SetSampleTime(pts100ns);
    sample->SetSampleDuration(duration100ns);
    hr = m_writer->WriteSample(m_stream, sample.Get());
    if(FAILED(hr)){
        // Throttle error log
        static int s_err = 0;
        if(s_err++ < 5) geode::log::error("WriteSample failed {}", (int)hr);
        return false;
    }
    m_frameIdx++;
    // Throttled success log
    if(m_frameIdx % (m_fps * 2) == 0){
        geode::log::info("KRecorder: Frame encoded #{}", (int)m_frameIdx);
    } else if(m_frameIdx <= 3){
        geode::log::info("KRecorder: Frame encoded #{}", (int)m_frameIdx);
    }
    return true;
#else
    return false;
#endif
}
bool WMFEncoder::flush(){
#ifdef _WIN32
    if(m_writer) m_writer->Flush(m_stream);
    return true;
#else
    return false;
#endif
}
void WMFEncoder::shutdown(){
#ifdef _WIN32
    if(m_writer){
        HRESULT hr = m_writer->Finalize();
        if(FAILED(hr)) geode::log::warn("Finalize failed {}", (int)hr);
        else geode::log::info("KRecorder: File finalized");
        m_writer.Reset();
    }
    MFShutdown();
    m_inited=false;
    m_frameIdx=0;
#endif
}
