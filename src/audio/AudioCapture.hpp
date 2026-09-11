#pragma once
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <functional>

struct AudioDeviceInfo {
    std::string id;
    std::string name;
};

class AudioCapture {
public:
    static std::vector<AudioDeviceInfo> enumerateMicDevices();
    static void logAvailableMics();

    // Callback for PCM data: pcm 48kHz 16-bit stereo
    using PCMCallback = std::function<void(const int16_t* data, size_t frames)>;

    bool startGameAudio(PCMCallback cb);
    bool startMic(const std::string& deviceIdOrName, PCMCallback cb);
    void stop();

private:
    std::thread m_gameThread;
    std::thread m_micThread;
    std::atomic<bool> m_running{false};
#ifdef _WIN32
    void* m_gameClient = nullptr;
    void* m_micClient = nullptr;
#endif
};
