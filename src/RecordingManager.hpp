#pragma once
#include "capture/IFrameCapture.hpp"
#include "encoder/IVideoEncoder.hpp"
#include "audio/AudioCapture.hpp"
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <string>
#include <chrono>
enum class RecState { Idle, Recording, Error };
class RecordingManager {
public:
    static RecordingManager& get();
    bool start();
    void stop();
    void onFrame(const Frame& f);
    // Called from main thread each frame: handles throttling + capture -> onFrame
    void captureTick();
    RecState getState() const { return m_state; }
    std::string getLastError() const { return m_lastError; }
    bool isRecording() const { return m_state==RecState::Recording; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    int getFPS() const { return m_fps; }
private:
    void worker();
    void captureLoop();
    RecState m_state=RecState::Idle;
    std::string m_lastError;
    std::queue<Frame> m_queue;
    std::mutex m_mutex;
    std::thread m_thread;
    std::thread m_captureThread;
    std::atomic<bool> m_running{false};
    std::unique_ptr<IVideoEncoder> m_encoder;
    std::unique_ptr<IFrameCapture> m_capture;
    AudioCapture m_audioCapture;
    std::string m_outPath;
    static constexpr size_t MAX_QUEUE=30;
    // capture timing
    int m_width=0, m_height=0, m_fps=60;
    std::chrono::steady_clock::time_point m_lastCapture{};
    uint64_t m_capturedFrames=0;
    uint64_t m_droppedFrames=0;
    std::string m_audioMode;
};
