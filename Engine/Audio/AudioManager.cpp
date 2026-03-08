#include "AudioManager.h"
#include "../Utils/Logger.h"
#include <xaudio2.h>
#include <algorithm>

class FVoiceCallback : public IXAudio2VoiceCallback
{
public:
    FVoiceCallback(KAudioManager* InManager, uint32_t InVoiceId)
        : Manager(InManager), VoiceId(InVoiceId) {}

    STDMETHOD_(void, OnBufferStart)(void*) override {}
    STDMETHOD_(void, OnBufferEnd)(void*) override {}
    STDMETHOD_(void, OnLoopEnd)(void*) override {}
    STDMETHOD_(void, OnStreamEnd)() override 
    { 
        if (Manager)
        {
            Manager->OnVoiceEnded(VoiceId);
        }
    }
    STDMETHOD_(void, OnVoiceError)(void*, HRESULT) override {}
    STDMETHOD_(void, OnVoiceProcessingPassStart)(UINT32) override {}
    STDMETHOD_(void, OnVoiceProcessingPassEnd)() override {}

private:
    KAudioManager* Manager;
    uint32_t VoiceId;
};

KAudioManager* KAudioManager::Instance = nullptr;

KAudioManager::KAudioManager()
    : XAudio2(nullptr)
    , MasteringVoice(nullptr)
    , NextVoiceId(1)
    , bIsInitialized(false)
{
    Instance = this;
}

KAudioManager::~KAudioManager()
{
    Shutdown();
    if (Instance == this)
    {
        Instance = nullptr;
    }
}

HRESULT KAudioManager::Initialize(const FAudioConfig& InConfig)
{
    if (bIsInitialized)
    {
        return S_OK;
    }

    Config = InConfig;

    HRESULT hr = XAudio2Create(&XAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create XAudio2 instance");
        return hr;
    }

    hr = XAudio2->CreateMasteringVoice(&MasteringVoice);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create mastering voice");
        XAudio2->Release();
        XAudio2 = nullptr;
        return hr;
    }

    SetMasterVolume(Config.MasterVolume);

    bIsInitialized = true;
    LOG_INFO("Audio manager initialized");
    return S_OK;
}

void KAudioManager::Shutdown()
{
    if (!bIsInitialized)
    {
        return;
    }

    StopAllSounds();

    for (auto& Pair : ActiveVoices)
    {
        FActiveVoice& Voice = Pair.second;
        if (Voice.SourceVoice)
        {
            Voice.SourceVoice->Stop();
            Voice.SourceVoice->FlushSourceBuffers();
            Voice.SourceVoice->DestroyVoice();
        }
        if (Voice.Callback)
        {
            delete Voice.Callback;
        }
    }
    ActiveVoices.clear();

    if (MasteringVoice)
    {
        MasteringVoice->DestroyVoice();
        MasteringVoice = nullptr;
    }

    if (XAudio2)
    {
        XAudio2->StopEngine();
        XAudio2->Release();
        XAudio2 = nullptr;
    }

    Sounds.clear();
    bIsInitialized = false;
    LOG_INFO("Audio manager shutdown");
}

void KAudioManager::Update(float DeltaTime)
{
    if (!bIsInitialized)
    {
        return;
    }

    std::vector<uint32_t> VoicesToRemove;

    for (const auto& Pair : ActiveVoices)
    {
        const FActiveVoice& Voice = Pair.second;
        if (!Voice.bIsPaused && Voice.SourceVoice)
        {
            XAUDIO2_VOICE_STATE State;
            Voice.SourceVoice->GetState(&State);

            if (State.BuffersQueued == 0 && !Voice.Params.bLooping)
            {
                VoicesToRemove.push_back(Pair.first);
            }
        }
    }

    for (uint32_t VoiceId : VoicesToRemove)
    {
        auto It = ActiveVoices.find(VoiceId);
        if (It != ActiveVoices.end())
        {
            if (It->second.SourceVoice)
            {
                It->second.SourceVoice->DestroyVoice();
            }
            if (It->second.Callback)
            {
                delete It->second.Callback;
            }
            ActiveVoices.erase(It);
        }
    }
}

std::shared_ptr<KSound> KAudioManager::LoadSound(const std::wstring& FilePath, const std::string& Name)
{
    std::string SoundName = Name;
    if (SoundName.empty())
    {
        SoundName = std::string(FilePath.begin(), FilePath.end());
    }

    auto ExistingSound = GetSound(SoundName);
    if (ExistingSound)
    {
        return ExistingSound;
    }

    auto NewSound = std::make_shared<KSound>();
    if (!NewSound->LoadFromWavFile(FilePath))
    {
        return nullptr;
    }

    NewSound->SetName(SoundName);
    Sounds[SoundName] = NewSound;

    LOG_INFO("Loaded sound: " + SoundName);
    return NewSound;
}

std::shared_ptr<KSound> KAudioManager::GetSound(const std::string& Name) const
{
    auto It = Sounds.find(Name);
    if (It != Sounds.end())
    {
        return It->second;
    }
    return nullptr;
}

void KAudioManager::UnloadSound(const std::string& Name)
{
    StopSound(Name);
    Sounds.erase(Name);
    LOG_INFO("Unloaded sound: " + Name);
}

void KAudioManager::UnloadAllSounds()
{
    StopAllSounds();
    Sounds.clear();
    LOG_INFO("Unloaded all sounds");
}

uint32_t KAudioManager::PlaySound(const std::string& Name, const FSoundParams& Params)
{
    if (!bIsInitialized)
    {
        return 0;
    }

    auto Sound = GetSound(Name);
    if (!Sound)
    {
        LOG_WARNING("Sound not found: " + Name);
        return 0;
    }

    return PlaySoundInternal(Sound, Params);
}

uint32_t KAudioManager::PlaySoundInternal(std::shared_ptr<KSound> Sound, const FSoundParams& Params)
{
    if (!Sound || !Sound->GetWaveFormat())
    {
        return 0;
    }

    IXAudio2SourceVoice* SourceVoice = nullptr;
    FVoiceCallback* Callback = new FVoiceCallback(this, NextVoiceId);

    HRESULT hr = XAudio2->CreateSourceVoice(
        &SourceVoice,
        reinterpret_cast<WAVEFORMATEX*>(Sound->GetWaveFormat()),
        0,
        XAUDIO2_DEFAULT_FREQ_RATIO,
        Callback
    );

    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create source voice");
        delete Callback;
        return 0;
    }

    XAUDIO2_BUFFER Buffer = {};
    Buffer.pAudioData = Sound->GetAudioData().data();
    Buffer.AudioBytes = static_cast<UINT32>(Sound->GetAudioData().size());
    Buffer.Flags = XAUDIO2_END_OF_STREAM;

    if (Params.bLooping)
    {
        Buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
    }

    hr = SourceVoice->SubmitSourceBuffer(&Buffer);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to submit source buffer");
        SourceVoice->DestroyVoice();
        delete Callback;
        return 0;
    }

    float Volume = Params.Volume;
    if (Params.bIsMusic)
    {
        Volume *= Config.MusicVolume;
    }
    else
    {
        Volume *= Config.SoundVolume;
    }
    SourceVoice->SetVolume(Volume);
    SourceVoice->SetFrequencyRatio(Params.Pitch);

    hr = SourceVoice->Start();
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to start voice");
        SourceVoice->DestroyVoice();
        delete Callback;
        return 0;
    }

    uint32_t VoiceId = NextVoiceId++;

    FActiveVoice ActiveVoice;
    ActiveVoice.VoiceId = VoiceId;
    ActiveVoice.SourceVoice = SourceVoice;
    ActiveVoice.Sound = Sound;
    ActiveVoice.Params = Params;
    ActiveVoice.bIsMusic = Params.bIsMusic;
    ActiveVoice.bIsPaused = false;
    ActiveVoice.Callback = Callback;

    ActiveVoices[VoiceId] = ActiveVoice;

    return VoiceId;
}

void KAudioManager::StopSound(uint32_t VoiceId)
{
    auto It = ActiveVoices.find(VoiceId);
    if (It != ActiveVoices.end())
    {
        if (It->second.SourceVoice)
        {
            It->second.SourceVoice->Stop();
            It->second.SourceVoice->FlushSourceBuffers();
            It->second.SourceVoice->DestroyVoice();
        }
        if (It->second.Callback)
        {
            delete It->second.Callback;
        }
        ActiveVoices.erase(It);
    }
}

void KAudioManager::StopSound(const std::string& Name)
{
    std::vector<uint32_t> VoicesToRemove;

    for (const auto& Pair : ActiveVoices)
    {
        if (Pair.second.Sound && Pair.second.Sound->GetName() == Name)
        {
            VoicesToRemove.push_back(Pair.first);
        }
    }

    for (uint32_t VoiceId : VoicesToRemove)
    {
        StopSound(VoiceId);
    }
}

void KAudioManager::StopAllSounds()
{
    for (auto& Pair : ActiveVoices)
    {
        if (Pair.second.SourceVoice)
        {
            Pair.second.SourceVoice->Stop();
            Pair.second.SourceVoice->FlushSourceBuffers();
            Pair.second.SourceVoice->DestroyVoice();
        }
        if (Pair.second.Callback)
        {
            delete Pair.second.Callback;
        }
    }
    ActiveVoices.clear();
}

void KAudioManager::PauseSound(uint32_t VoiceId)
{
    auto It = ActiveVoices.find(VoiceId);
    if (It != ActiveVoices.end())
    {
        if (It->second.SourceVoice)
        {
            It->second.SourceVoice->Stop();
        }
        It->second.bIsPaused = true;
    }
}

void KAudioManager::ResumeSound(uint32_t VoiceId)
{
    auto It = ActiveVoices.find(VoiceId);
    if (It != ActiveVoices.end())
    {
        if (It->second.SourceVoice)
        {
            It->second.SourceVoice->Start();
        }
        It->second.bIsPaused = false;
    }
}

void KAudioManager::PauseAllSounds()
{
    for (auto& Pair : ActiveVoices)
    {
        if (Pair.second.SourceVoice)
        {
            Pair.second.SourceVoice->Stop();
        }
        Pair.second.bIsPaused = true;
    }
}

void KAudioManager::ResumeAllSounds()
{
    for (auto& Pair : ActiveVoices)
    {
        if (Pair.second.SourceVoice)
        {
            Pair.second.SourceVoice->Start();
        }
        Pair.second.bIsPaused = false;
    }
}

void KAudioManager::SetMasterVolume(float Volume)
{
    Config.MasterVolume = std::max(0.0f, std::min(1.0f, Volume));
    if (MasteringVoice)
    {
        MasteringVoice->SetVolume(Config.MasterVolume);
    }
}

void KAudioManager::SetSoundVolume(float Volume)
{
    Config.SoundVolume = std::max(0.0f, std::min(1.0f, Volume));
}

void KAudioManager::SetMusicVolume(float Volume)
{
    Config.MusicVolume = std::max(0.0f, std::min(1.0f, Volume));

    for (auto& Pair : ActiveVoices)
    {
        if (Pair.second.bIsMusic && Pair.second.SourceVoice)
        {
            float NewVolume = Pair.second.Params.Volume * Config.MusicVolume;
            Pair.second.SourceVoice->SetVolume(NewVolume);
        }
    }
}

bool KAudioManager::IsSoundPlaying(uint32_t VoiceId) const
{
    auto It = ActiveVoices.find(VoiceId);
    if (It != ActiveVoices.end())
    {
        XAUDIO2_VOICE_STATE State;
        It->second.SourceVoice->GetState(&State);
        return State.BuffersQueued > 0 || It->second.Params.bLooping;
    }
    return false;
}

bool KAudioManager::IsSoundPlaying(const std::string& Name) const
{
    for (const auto& Pair : ActiveVoices)
    {
        if (Pair.second.Sound && Pair.second.Sound->GetName() == Name)
        {
            XAUDIO2_VOICE_STATE State;
            Pair.second.SourceVoice->GetState(&State);
            return State.BuffersQueued > 0 || Pair.second.Params.bLooping;
        }
    }
    return false;
}

float KAudioManager::GetSoundDuration(uint32_t VoiceId) const
{
    auto It = ActiveVoices.find(VoiceId);
    if (It != ActiveVoices.end() && It->second.Sound)
    {
        return It->second.Sound->GetDuration();
    }
    return 0.0f;
}

float KAudioManager::GetSoundCurrentTime(uint32_t VoiceId) const
{
    auto It = ActiveVoices.find(VoiceId);
    if (It != ActiveVoices.end() && It->second.SourceVoice && It->second.Sound)
    {
        XAUDIO2_VOICE_STATE State;
        It->second.SourceVoice->GetState(&State);
        
        float BytesPerSecond = static_cast<float>(It->second.Sound->GetSampleRate() * 
                                                   It->second.Sound->GetChannels() * 
                                                   (It->second.Sound->GetBitsPerSample() / 8));
        if (BytesPerSecond > 0)
        {
            float BytesPlayed = static_cast<float>(State.SamplesPlayed) * 
                               It->second.Sound->GetChannels() * 
                               (It->second.Sound->GetBitsPerSample() / 8);
            return BytesPlayed / BytesPerSecond;
        }
    }
    return 0.0f;
}

void KAudioManager::SetListenerParams(const FListenerParams& Params)
{
    ListenerParams = Params;
}

void KAudioManager::OnVoiceEnded(uint32_t VoiceId)
{
    auto It = ActiveVoices.find(VoiceId);
    if (It != ActiveVoices.end() && !It->second.Params.bLooping)
    {
        if (It->second.SourceVoice)
        {
            It->second.SourceVoice->DestroyVoice();
        }
        if (It->second.Callback)
        {
            delete It->second.Callback;
        }
        ActiveVoices.erase(It);
    }
}
