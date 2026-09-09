#pragma once
#include <string>
#include <functional>
#include <chrono>
class HotkeyManager {
public:
    static HotkeyManager& get();
    void init(std::function<void()> toggle);
    void shutdown();
    bool isKeyDown(int vk) const;
    bool shouldToggle(int vk);
private:
    std::function<void()> m_cb;
    std::chrono::steady_clock::time_point m_lastToggle{};
};
int vkFromString(const std::string& s);
