#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <xaudio2.h>
#include "AudioTypes.h"
#include "Sound.h"
#include <memory>
#include <unordered_map>
#include <string>

class KAudioManager
{
public:
    KAudioManager();
    ~KAudioManager();

    KAudioManager(const KAudioManager&) = delete;
    KAudioManager& operator=(const KAudioManager&) = delete;

    HRESULT Initialize(const FAudioConfig& InConfig = FAudioConfig());
    void Shutdown();

    void Update(float DeltaTime);

    std::shared_ptr<KSound> LoadSound(const std::wstring& FilePath, const std::string& Name = "");
    std::shared_ptr<KSound> GetSound(const std::string& Name) const;
    void UnloadSound(const std::string& Name);
    void UnloadAllSounds();

    uint32_t PlaySound(const std::string& Name, const FSoundParams& Params = FSoundParams());
    void StopSound(uint32_t VoiceId);
    void StopSound(const std::string& Name);
    void StopAllSounds();

    void PauseSound(uint32_t VoiceId);
    void ResumeSound(uint32_t VoiceId);
    void PauseAllSounds();
    void ResumeAllSounds();

    void SetMasterVolume(float Volume);
    float GetMasterVolume() const { return Config.MasterVolume; }

    void SetSoundVolume(float Volume);
    float GetSoundVolume() const { return Config.SoundVolume; }

    void SetMusicVolume(float Volume);
    float GetMusicVolume() const { return Config.MusicVolume; }

    bool IsSoundPlaying(uint32_t VoiceId) const;
    bool IsSoundPlaying(const std::string& Name) const;

    float GetSoundDuration(uint32_t VoiceId) const;
    float GetSoundCurrentTime(uint32_t VoiceId) const;

    void SetListenerParams(const FListenerParams& Params);

    uint32_t GetActiveVoiceCount() const { return static_cast<uint32_t>(ActiveVoices.size()); }
    uint32_t GetLoadedSoundCount() const { return static_cast<uint32_t>(Sounds.size()); }

    static KAudioManager* GetInstance() { return Instance; }

    void OnVoiceEnded(uint32_t VoiceId);

private:
    struct FActiveVoice
    {
        uint32_t VoiceId = 0;
        IXAudio2SourceVoice* SourceVoice = nullptr;
        std::shared_ptr<KSound> Sound;
        FSoundParams Params;
        bool bIsMusic = false;
        bool bIsPaused = false;
        IXAudio2VoiceCallback* Callback = nullptr;
    };

    uint32_t PlaySoundInternal(std::shared_ptr<KSound> Sound, const FSoundParams& Params);

private:
    IXAudio2* XAudio2;
    IXAudio2MasteringVoice* MasteringVoice;

    FAudioConfig Config;
    FListenerParams ListenerParams;

    std::unordered_map<std::string, std::shared_ptr<KSound>> Sounds;
    std::unordered_map<uint32_t, FActiveVoice> ActiveVoices;
    std::vector<uint32_t> PendingVoiceCleanup;
    CRITICAL_SECTION AudioLock;

    uint32_t NextVoiceId;
    bool bIsInitialized;

    static KAudioManager* Instance;
};
