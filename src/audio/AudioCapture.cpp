#include "AudioCapture.hpp"
#include <Geode/Geode.hpp>
#ifdef _WIN32
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <avrt.h>
#pragma comment(lib,"avrt.lib")
#pragma comment(lib,"ole32.lib")
#endif

std::vector<AudioDeviceInfo> AudioCapture::enumerateMicDevices(){
    std::vector<AudioDeviceInfo> out;
#ifdef _WIN32
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    IMMDeviceEnumerator* pEnum = nullptr;
    if(FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnum))) {
        return out;
    }
    IMMDeviceCollection* pColl = nullptr;
    if(SUCCEEDED(pEnum->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &pColl))){
        UINT count = 0; pColl->GetCount(&count);
        for(UINT i=0;i<count;++i){
            IMMDevice* pDev = nullptr;
            if(SUCCEEDED(pColl->Item(i, &pDev))){
                LPWSTR id = nullptr; pDev->GetId(&id);
                IPropertyStore* pProps = nullptr;
                std::string name = "Mic " + std::to_string(i);
                if(SUCCEEDED(pDev->OpenPropertyStore(STGM_READ, &pProps))){
                    PROPVARIANT var; PropVariantInit(&var);
                    if(SUCCEEDED(pProps->GetValue(PKEY_Device_FriendlyName, &var)) && var.vt==VT_LPWSTR){
                        std::wstring w(var.pwszVal);
                        name = std::string(w.begin(), w.end());
                    }
                    PropVariantClear(&var);
                    pProps->Release();
                }
                std::wstring wid(id); std::string sid(wid.begin(), wid.end());
                out.push_back({sid, name});
                CoTaskMemFree(id);
                pDev->Release();
            }
        }
        pColl->Release();
    }
    // Also add default
    out.insert(out.begin(), {"default", "Default Microphone"});
    pEnum->Release();
#endif
    return out;
}

void AudioCapture::logAvailableMics(){
    auto devs = enumerateMicDevices();
    geode::log::info("KRecorder: found {} mic(s)", (int)devs.size());
    for(auto& d: devs) geode::log::info("  mic: '{}' id='{}'", d.name, d.id);
}

bool AudioCapture::startGameAudio(PCMCallback cb){
#ifdef _WIN32
    m_running = true;
    m_gameThread = std::thread([this, cb]{
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        geode::log::info("KRecorder: game audio loopback thread started");
        // Minimal WASAPI loopback: try to open default render device in loopback mode
        IMMDeviceEnumerator* pEnum = nullptr;
        if(FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnum))){
            geode::log::warn("KRecorder: game audio - no enumerator");
            return;
        }
        IMMDevice* pDev = nullptr;
        if(FAILED(pEnum->GetDefaultAudioEndpoint(eRender, eConsole, &pDev))){
            geode::log::warn("KRecorder: game audio - no default render");
            pEnum->Release(); return;
        }
        IAudioClient* pClient = nullptr;
        if(FAILED(pDev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&pClient))){
            pDev->Release(); pEnum->Release(); return;
        }
        WAVEFORMATEX* pwfx = nullptr; pClient->GetMixFormat(&pwfx);
        // Force 48k stereo for encoder
        WAVEFORMATEX fmt = *pwfx; fmt.nSamplesPerSec = 48000; fmt.nChannels = 2; fmt.nBlockAlign = 4; fmt.nAvgBytesPerSec = 192000; fmt.wBitsPerSample = 16;
        HRESULT hr = pClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 500000, 0, &fmt, nullptr);
        if(FAILED(hr)){
            geode::log::warn("KRecorder: game audio init failed {}", (int)hr);
            CoTaskMemFree(pwfx); pClient->Release(); pDev->Release(); pEnum->Release(); return;
        }
        IAudioCaptureClient* pCap = nullptr; pClient->GetService(__uuidof(IAudioCaptureClient), (void**)&pCap);
        pClient->Start();
        while(m_running){
            UINT32 frames = 0; pCap->GetNextPacketSize(&frames);
            if(frames==0){ Sleep(5); continue; }
            BYTE* data=nullptr; UINT32 avail=0; DWORD flags=0; UINT64 pos=0, qpc=0;
            if(SUCCEEDED(pCap->GetBuffer(&data,&avail,&flags,&pos,&qpc)) && avail>0 && data){
                // Convert float if needed - for now assume 16-bit
                // If pwfx is float, convert to int16
                cb((int16_t*)data, avail);
                pCap->ReleaseBuffer(avail);
            } else Sleep(2);
        }
        pClient->Stop(); pCap->Release(); pClient->Release(); CoTaskMemFree(pwfx); pDev->Release(); pEnum->Release();
        geode::log::info("KRecorder: game audio thread stopped");
    });
    return true;
#else
    return false;
#endif
}

bool AudioCapture::startMic(const std::string& deviceIdOrName, PCMCallback cb){
#ifdef _WIN32
    m_running = true;
    std::string target = deviceIdOrName;
    m_micThread = std::thread([this, target, cb]{
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        geode::log::info("KRecorder: mic thread started device='{}'", target);
        IMMDeviceEnumerator* pEnum = nullptr;
        CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnum);
        IMMDevice* pDev = nullptr;
        if(target.empty() || target=="default"){
            pEnum->GetDefaultAudioEndpoint(eCapture, eConsole, &pDev);
        } else {
            // Find by id or name
            IMMDeviceCollection* pColl = nullptr; pEnum->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &pColl);
            UINT cnt=0; pColl->GetCount(&cnt);
            for(UINT i=0;i<cnt;++i){
                IMMDevice* d=nullptr; pColl->Item(i,&d);
                LPWSTR id=nullptr; d->GetId(&id);
                std::wstring wid(id); std::string sid(wid.begin(), wid.end());
                IPropertyStore* ps=nullptr; d->OpenPropertyStore(STGM_READ,&ps);
                std::string name;
                if(ps){ PROPVARIANT v; PropVariantInit(&v); if(SUCCEEDED(ps->GetValue(PKEY_Device_FriendlyName,&v))){ std::wstring w(v.pwszVal); name=std::string(w.begin(),w.end()); } PropVariantClear(&v); ps->Release(); }
                if(sid==target || name==target){ pDev=d; CoTaskMemFree(id); break; }
                CoTaskMemFree(id); d->Release();
            }
            if(pColl) pColl->Release();
            if(!pDev) pEnum->GetDefaultAudioEndpoint(eCapture, eConsole, &pDev);
        }
        if(!pDev){ geode::log::warn("KRecorder: mic no device"); pEnum->Release(); return; }
        IAudioClient* pClient=nullptr; pDev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&pClient);
        WAVEFORMATEX* pwfx=nullptr; pClient->GetMixFormat(&pwfx);
        WAVEFORMATEX fmt=*pwfx; fmt.nSamplesPerSec=48000; fmt.nChannels=1; fmt.nBlockAlign=2; fmt.nAvgBytesPerSec=96000; fmt.wBitsPerSample=16;
        if(FAILED(pClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 500000, 0, &fmt, nullptr))){
            CoTaskMemFree(pwfx); pClient->Release(); pDev->Release(); pEnum->Release(); return;
        }
        IAudioCaptureClient* pCap=nullptr; pClient->GetService(__uuidof(IAudioCaptureClient),(void**)&pCap);
        pClient->Start();
        while(m_running){
            UINT32 frames=0; pCap->GetNextPacketSize(&frames);
            if(frames==0){ Sleep(5); continue; }
            BYTE* data=nullptr; UINT32 avail=0; DWORD flags=0;
            if(SUCCEEDED(pCap->GetBuffer(&data,&avail,&flags,nullptr,nullptr)) && avail>0){
                cb((int16_t*)data, avail);
                pCap->ReleaseBuffer(avail);
            } else Sleep(2);
        }
        pClient->Stop(); pCap->Release(); pClient->Release(); CoTaskMemFree(pwfx); pDev->Release(); pEnum->Release();
    });
    return true;
#else
    return false;
#endif
}

void AudioCapture::stop(){
    m_running = false;
    if(m_gameThread.joinable()) m_gameThread.join();
    if(m_micThread.joinable()) m_micThread.join();
}
