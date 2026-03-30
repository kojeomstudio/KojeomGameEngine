#pragma once

#include "../Core/Subsystem.h"
#include "AudioManager.h"
#include <memory>

/**
 * @brief Audio subsystem adapter for the modular engine architecture
 *
 * Wraps KAudioManager as an ISubsystem, enabling it to be registered
 * with KSubsystemRegistry for uniform lifecycle management.
 */
class KAudioSubsystem : public ISubsystem
{
public:
    KAudioSubsystem() = default;
    ~KAudioSubsystem() override = default;

    KAudioSubsystem(const KAudioSubsystem&) = delete;
    KAudioSubsystem& operator=(const KAudioSubsystem&) = delete;

    HRESULT Initialize() override
    {
        HRESULT hr = AudioManager.Initialize(AudioConfig);
        if (SUCCEEDED(hr))
        {
            State = ESubsystemState::Initialized;
        }
        return hr;
    }

    void Tick(float DeltaTime) override
    {
        AudioManager.Update(DeltaTime);
    }

    void Shutdown() override
    {
        AudioManager.Shutdown();
        State = ESubsystemState::Shutdown;
    }

    const std::string& GetName() const override
    {
        static const std::string Name = "AudioSubsystem";
        return Name;
    }

    /** @brief Access the underlying audio manager */
    KAudioManager* GetAudioManager() { return &AudioManager; }
    const KAudioManager* GetAudioManager() const { return &AudioManager; }

    /** @brief Set audio configuration (must be called before Initialize) */
    void SetConfig(const FAudioConfig& InConfig) { AudioConfig = InConfig; }
    const FAudioConfig& GetConfig() const { return AudioConfig; }

private:
    KAudioManager AudioManager;
    FAudioConfig AudioConfig;
};
