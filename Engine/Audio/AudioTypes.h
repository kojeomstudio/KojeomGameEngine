#pragma once

#include <cstdint>
#include <string>
#include <cmath>

namespace EAudioFormat
{
    enum Format : uint8_t
    {
        Unknown = 0,
        Mono8 = 1,
        Mono16 = 2,
        Stereo8 = 3,
        Stereo16 = 4,
    };
}

struct FAudioConfig
{
    uint32_t SampleRate = 44100;
    uint32_t Channels = 2;
    uint32_t BitsPerSample = 16;
    uint32_t BufferSize = 4096;
    float MasterVolume = 1.0f;
    float SoundVolume = 1.0f;
    float MusicVolume = 1.0f;
};

struct FSoundParams
{
    std::string Name;
    std::string FilePath;
    float Volume = 1.0f;
    float Pitch = 1.0f;
    float Pan = 0.0f;
    bool bLooping = false;
    bool bIs3D = false;
    bool bIsMusic = false;
};

struct FSoundState
{
    bool bIsPlaying = false;
    bool bIsPaused = false;
    float CurrentTime = 0.0f;
    float Duration = 0.0f;
};

struct FListenerParams
{
    float Position[3] = { 0.0f, 0.0f, 0.0f };
    float Velocity[3] = { 0.0f, 0.0f, 0.0f };
    float Forward[3] = { 0.0f, 0.0f, 1.0f };
    float Up[3] = { 0.0f, 1.0f, 0.0f };
};

struct FSound3DParams
{
    float Position[3] = { 0.0f, 0.0f, 0.0f };
    float Velocity[3] = { 0.0f, 0.0f, 0.0f };
    float MinDistance = 1.0f;
    float MaxDistance = 100.0f;
    float RolloffFactor = 1.0f;
};
