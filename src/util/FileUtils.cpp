#include "FileUtils.hpp"
#include "Settings.hpp"
#include <Geode/Geode.hpp>
#include <chrono>
#include <iomanip>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#endif
std::string FileUtils::getDefaultOutputDir(){
#ifdef _WIN32
    PWSTR p=nullptr;
    // Primary: Desktop (user request)
    if(SHGetKnownFolderPath(FOLDERID_Desktop,0,nullptr,&p)==S_OK){
        std::wstring ws(p); CoTaskMemFree(p);
        std::string s(ws.begin(), ws.end());
        s += "\\KRecorder_Recordings";
        std::filesystem::create_directories(s);
        return s;
    }
    p=nullptr;
    if(SHGetKnownFolderPath(FOLDERID_Videos,0,nullptr,&p)==S_OK){
        std::wstring ws(p); CoTaskMemFree(p);
        std::string s(ws.begin(), ws.end());
        return s + "\\KRecorder_Recordings";
    }
#endif
    auto dir = geode::Mod::get()->getSaveDir() / "videos";
    std::filesystem::create_directories(dir);
    return dir.string();
}
std::string FileUtils::generateFilename(){
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{}; localtime_s(&tm,&t);
    std::string fmt = "mp4";
    try { fmt = KRecorderSettings::getFormat(); } catch(...) {}
    if(fmt != "mkv" && fmt != "mp4") fmt = "mp4";
    std::ostringstream oss;
    oss << "KRecorder_" << std::put_time(&tm,"%Y-%m-%d_%H-%M-%S") << "." << fmt;
    return oss.str();
}
bool FileUtils::ensureDir(const std::string& p){
    std::error_code ec;
    return std::filesystem::create_directories(p, ec) || std::filesystem::exists(p);
}
bool FileUtils::hasEnoughSpace(const std::string& dir, uint64_t need){
#ifdef _WIN32
    ULARGE_INTEGER freeBytes;
    if(GetDiskFreeSpaceExA(dir.c_str(), &freeBytes,nullptr,nullptr)) return freeBytes.QuadPart > need;
#endif
    return true;
}
