#pragma once

#include "AudioTypes.h"
#include <memory>
#include <vector>
#include <string>

struct IXAudio2SourceVoice;

class KSound
{
public:
    KSound();
    ~KSound();

    KSound(const KSound&) = delete;
    KSound& operator=(const KSound&) = delete;

    bool LoadFromWavFile(const std::wstring& FilePath);
    bool LoadFromMemory(const uint8_t* Data, size_t Size, uint32_t SampleRate, uint32_t Channels, uint32_t BitsPerSample);

    void SetVolume(float Volume);
    float GetVolume() const { return Volume; }

    void SetPitch(float Pitch);
    float GetPitch() const { return Pitch; }

    void SetPan(float Pan);
    float GetPan() const { return Pan; }

    void SetLooping(bool bLoop) { bIsLooping = bLoop; }
    bool IsLooping() const { return bIsLooping; }

    float GetDuration() const { return Duration; }

    const std::string& GetName() const { return Name; }
    void SetName(const std::string& InName) { Name = InName; }

    uint32_t GetSampleRate() const { return SampleRate; }
    uint32_t GetChannels() const { return Channels; }
    uint32_t GetBitsPerSample() const { return BitsPerSample; }

    const std::vector<uint8_t>& GetAudioData() const { return AudioData; }

    void* GetWaveFormat() const { return WaveFormat.get(); }

private:
    bool ParseWavFile(const std::wstring& FilePath);
    void CalculateDuration();

private:
    std::string Name;
    std::vector<uint8_t> AudioData;
    std::unique_ptr<uint8_t[]> WaveFormat;

    uint32_t SampleRate;
    uint32_t Channels;
    uint32_t BitsPerSample;
    uint32_t ByteRate;
    uint32_t BlockAlign;
    uint32_t DataSize;

    float Volume;
    float Pitch;
    float Pan;
    float Duration;
    bool bIsLooping;
    bool bIs3D;
};
