#include "RecordingManager.hpp"
#include "Settings.hpp"
#include "Overlay.hpp"
#include "util/FileUtils.hpp"
#include "encoder/WMFEncoder.hpp"
#include <Geode/Geode.hpp>

// Factory from DesktopDuplication.cpp
extern "C" IFrameCapture* createDesktopDuplicationCapture();

RecordingManager& RecordingManager::get(){ static RecordingManager i; return i; }

bool RecordingManager::start(){
    if(m_state==RecState::Recording) return false;
    if(!KRecorderSettings::isRecordingEnabled()){ m_lastError="Recording disabled"; return false; }
    geode::log::info("KRecorder: F9 detected -> Starting recorder");
    std::string dir = KRecorderSettings::getOutputDir();
    if(dir.empty()) dir = FileUtils::getDefaultOutputDir();
    if(!FileUtils::ensureDir(dir)){ m_lastError="Cannot create output dir: "+dir; geode::log::error("{}",m_lastError); return false; }
    if(!FileUtils::hasEnoughSpace(dir, 50ULL*1024*1024)){ m_lastError="Insufficient disk space"; geode::Notification::create("KRecorder: not enough disk space", geode::NotificationIcon::Error)->show(); return false; }
    std::string file = dir + "\\" + FileUtils::generateFilename();
    int w = KRecorderSettings::getWidth();
    int h = KRecorderSettings::getHeight();
    if(w==0||h==0){ auto ws = cocos2d::CCDirector::sharedDirector()->getWinSize(); w=(int)ws.width; h=(int)ws.height; }
    // clamp to even dimensions for H.264
    if(w%2) w--; if(h%2) h--;
    if(w<64) w=1280; if(h<64) h=720;
    int fps = KRecorderSettings::getFPS();
    int br = KRecorderSettings::getBitrate();
    m_width=w; m_height=h; m_fps=fps;

    // Init capture backend
    m_capture.reset(createDesktopDuplicationCapture());
    if(!m_capture->init(w,h)){
        m_lastError="Capture init failed";
        geode::log::error("KRecorder: Capture initialized FAILED");
        geode::Notification::create("KRecorder: capture init failed", geode::NotificationIcon::Error)->show();
        m_capture.reset();
        return false;
    }
    geode::log::info("KRecorder: Capture initialized {}x{} @{}fps", w, h, fps);

    m_encoder = std::make_unique<WMFEncoder>();
    if(!m_encoder->configure(w,h,fps,br,file)){
        m_lastError="Encoder init failed";
        geode::Notification::create("KRecorder: encoder failed", geode::NotificationIcon::Error)->show();
        m_capture->shutdown(); m_capture.reset();
        m_encoder.reset();
        return false;
    }
    geode::log::info("KRecorder: Encoder initialized -> {}", file);
    m_outPath=file;
    m_running=true;
    m_capturedFrames=0;
    m_lastCapture = std::chrono::steady_clock::now();
    m_thread = std::thread(&RecordingManager::worker, this);
    m_state=RecState::Recording;
    RecordingOverlay::get().show();
    geode::Notification::create("KRecorder: recording started", geode::NotificationIcon::Success)->show();
    geode::log::info("KRecorder: Recording started -> {}", file);
    return true;
}

void RecordingManager::stop(){
    if(m_state!=RecState::Recording) return;
    geode::log::info("KRecorder: Stopping recorder ({} frames captured)", (int)m_capturedFrames);
    m_running=false;
    if(m_thread.joinable()) m_thread.join();
    if(m_encoder){ m_encoder->flush(); m_encoder->shutdown(); m_encoder.reset(); }
    if(m_capture){ m_capture->shutdown(); m_capture.reset(); }
    RecordingOverlay::get().hide();
    m_state=RecState::Idle;
    geode::Notification::create(("KRecorder: saved to " + m_outPath).c_str())->show();
    geode::log::info("KRecorder: File finalized -> {}", m_outPath);
}

void RecordingManager::onFrame(const Frame& f){
    if(m_state!=RecState::Recording) return;
    std::lock_guard<std::mutex> lk(m_mutex);
    if(m_queue.size() >= MAX_QUEUE){
        m_queue.pop();
        // throttle warn
        static int s_warn=0; if(s_warn++%60==0) geode::log::warn("Frame queue overflow, dropping frame");
    }
    m_queue.push(f);
}

void RecordingManager::captureTick(){
    if(m_state!=RecState::Recording || !m_capture) return;
    // Drop frames if encoder is falling behind (prevents main thread lag)
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        if(m_queue.size() > 30) return;
    }
    auto now = std::chrono::steady_clock::now();
    auto interval = std::chrono::microseconds(1000000 / m_fps);
    if(now - m_lastCapture < interval) return;
    m_lastCapture = now;

    // Hide overlay so it is NOT captured, capture, then show
    bool wasVisible = RecordingOverlay::get().isVisible();
    if(wasVisible) RecordingOverlay::get().hide();

    auto opt = m_capture->capture();

    if(wasVisible) RecordingOverlay::get().show();

    if(!opt){
        // no frame this tick (DXGI timeout) - not an error
        return;
    }
    // Fix size if capture returned slightly different (e.g., window resized) - drop
    if(opt->width != m_width || opt->height != m_height){
        geode::log::warn("KRecorder: captured size {}x{} != encoder {}x{} - re-init not supported, dropping", opt->width, opt->height, m_width, m_height);
        return;
    }
    m_capturedFrames++;
    if(m_capturedFrames <= 3 || m_capturedFrames % (m_fps*2) == 0){
        geode::log::info("KRecorder: Frame captured #{} (queue {})", (int)m_capturedFrames, (int)m_queue.size());
    }
    onFrame(*opt);
}

void RecordingManager::worker(){
    while(m_running || !m_queue.empty()){
        std::optional<Frame> f;
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            if(!m_queue.empty()){ f=m_queue.front(); m_queue.pop(); }
        }
        if(f){
            if(!m_encoder->encode(*f)) geode::log::error("encode failed");
            else {
                // throttled encode log is inside WMFEncoder
            }
        }
        else std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}
