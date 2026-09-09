#include "Hotkey.hpp"
#include "Settings.hpp"
#include <Geode/Geode.hpp>
#ifdef _WIN32
#include <windows.h>
#endif
int vkFromString(const std::string& s){
    if(s=="F1") return VK_F1; if(s=="F2") return VK_F2; if(s=="F3") return VK_F3; if(s=="F4") return VK_F4;
    if(s=="F5") return VK_F5; if(s=="F6") return VK_F6; if(s=="F7") return VK_F7; if(s=="F8") return VK_F8;
    if(s=="F9") return VK_F9; if(s=="F10") return VK_F10; if(s=="F11") return VK_F11; if(s=="F12") return VK_F12;
    if(s=="R") return 'R'; if(s=="P") return 'P';
    return VK_F9;
}
HotkeyManager& HotkeyManager::get(){ static HotkeyManager i; return i; }
void HotkeyManager::init(std::function<void()> toggle){
    m_cb=toggle;
    m_lastToggle = std::chrono::steady_clock::now() - std::chrono::seconds(1);
}
void HotkeyManager::shutdown(){ m_cb=nullptr; }
bool HotkeyManager::isKeyDown(int vk) const {
#ifdef _WIN32
    // GetAsyncKeyState returns 0x8000 when down; also check foreground window is GD to avoid global spam
    SHORT s = GetAsyncKeyState(vk);
    return (s & 0x8000) != 0;
#else
    return false;
#endif
}
bool HotkeyManager::shouldToggle(int vk){
    if(!isKeyDown(vk)) return false;
    auto now = std::chrono::steady_clock::now();
    if(now - m_lastToggle < std::chrono::milliseconds(350)) return false;
    m_lastToggle = now;
    return true;
}
