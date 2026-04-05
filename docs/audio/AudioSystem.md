# Audio System

## Overview

The KojeomGameEngine audio system is built on Microsoft [XAudio2](https://learn.microsoft.com/en-us/windows/win32/xaudio2/xaudio2-apis-portal), providing low-latency audio playback for WAV files with support for volume channels, pitch shifting, panning, looping, and 3D spatial audio parameters.

The system consists of three core components:

| Class | Header | Role |
|-------|--------|------|
| `KSound` | `Engine/Audio/Sound.h` | Represents a single audio clip (WAV data, format, playback properties) |
| `KAudioManager` | `Engine/Audio/AudioManager.h` | Singleton that owns the XAudio2 engine, manages voice pooling and sound lifecycle |
| `KAudioSubsystem` | `Engine/Audio/AudioSubsystem.h` | `ISubsystem` adapter that wraps `KAudioManager` for the modular subsystem registry |

Supporting types are defined in `Engine/Audio/AudioTypes.h`.

---

## Architecture

```
KAudioSubsystem  (ISubsystem adapter)
    └── KAudioManager  (singleton, owns XAudio2 engine)
            ├── IXAudio2              (COM engine instance)
            ├── IXAudio2MasteringVoice (master output)
            ├── Sounds map<string, shared_ptr<KSound>>
            └── ActiveVoices map<uint32, FActiveVoice>
                    ├── IXAudio2SourceVoice
                    └── FVoiceCallback (IXAudio2VoiceCallback)
```

### Voice Lifecycle

1. **Load** -- `KAudioManager::LoadSound()` reads a WAV file into a `shared_ptr<KSound>` stored by name.
2. **Play** -- `PlaySound()` creates an `IXAudio2SourceVoice`, submits the audio buffer, and starts playback. A unique `VoiceId` is returned.
3. **Update** -- `Update(DeltaTime)` polls each active voice; finished non-looping voices are destroyed and removed.
4. **Stop** -- `StopSound(VoiceId)` immediately stops, flushes, and destroys the source voice.
5. **Pause/Resume** -- Stops/restarts the source voice without destroying it.
6. **Callback** -- `FVoiceCallback::OnStreamEnd()` notifies the manager when a voice finishes naturally.

---

## KSound

`Engine/Audio/Sound.h`

Represents a loaded audio clip. Holds raw PCM data, wave format metadata, and per-instance playback properties.

### Loading

```cpp
bool LoadFromWavFile(const std::wstring& FilePath);
bool LoadFromMemory(const uint8_t* Data, size_t Size,
                    uint32_t SampleRate, uint32_t Channels, uint32_t BitsPerSample);
```

`LoadFromWavFile` parses standard RIFF/WAVE files, extracting PCM data and constructing an internal `WAVEFORMATEX`. `LoadFromMemory` accepts raw PCM data with explicit format parameters.

### Properties

| Method | Range | Default |
|--------|-------|---------|
| `SetVolume(float)` | `[0.0, 1.0]` | `1.0` |
| `SetPitch(float)` | `[0.5, 2.0]` | `1.0` |
| `SetPan(float)` | `[-1.0, 1.0]` | `0.0` |
| `SetLooping(bool)` | -- | `false` |

### Accessors

```cpp
float    GetDuration()     const;
float    GetVolume()       const;
float    GetPitch()        const;
float    GetPan()          const;
bool     IsLooping()       const;
uint32   GetSampleRate()   const;
uint32   GetChannels()     const;
uint32   GetBitsPerSample()const;
const std::vector<uint8_t>& GetAudioData() const;
void*    GetWaveFormat()   const;
```

---

## KAudioManager

`Engine/Audio/AudioManager.h`

Singleton audio manager. Owns the XAudio2 engine instance, manages sound loading, voice creation, and volume channels.

### Initialization

```cpp
HRESULT Initialize(const FAudioConfig& InConfig = FAudioConfig());
void    Shutdown();
```

Creates the `IXAudio2` engine and mastering voice. Call once at engine startup.

### Singleton Access

```cpp
static KAudioManager* GetInstance();
```

### Sound Loading

```cpp
std::shared_ptr<KSound> LoadSound(const std::wstring& FilePath, const std::string& Name = "");
std::shared_ptr<KSound> GetSound(const std::string& Name) const;
void UnloadSound(const std::string& Name);
void UnloadAllSounds();
```

Sounds are cached by name. If a sound with the same name is already loaded, `LoadSound` returns the existing instance.

### Playback

```cpp
uint32_t PlaySound(const std::string& Name, const FSoundParams& Params = FSoundParams());
void     StopSound(uint32_t VoiceId);
void     StopSound(const std::string& Name);
void     StopAllSounds();
```

`PlaySound` returns a `VoiceId` (non-zero on success) used to control that specific playback instance. The same sound can be played multiple times concurrently, each receiving a unique `VoiceId`.

### Pause and Resume

```cpp
void PauseSound(uint32_t VoiceId);
void ResumeSound(uint32_t VoiceId);
void PauseAllSounds();
void ResumeAllSounds();
```

Pause stops the source voice without destroying it. Resume restarts playback from where it was paused.

### Volume Channels

Three independent volume categories are applied multiplicatively:

```
Final Voice Volume = Params.Volume * ChannelVolume * MasterVolume
```

| Method | Description |
|--------|-------------|
| `SetMasterVolume(float)` | Global master volume (applied to the mastering voice) |
| `SetSoundVolume(float)` | Volume multiplier for non-music sounds |
| `SetMusicVolume(float)` | Volume multiplier for music sounds (also retroactively updates active music voices) |

### Querying Playback State

```cpp
bool  IsSoundPlaying(uint32_t VoiceId) const;
bool  IsSoundPlaying(const std::string& Name) const;
float GetSoundDuration(uint32_t VoiceId) const;
float GetSoundCurrentTime(uint32_t VoiceId) const;
uint32_t GetActiveVoiceCount() const;
uint32_t GetLoadedSoundCount() const;
```

### 3D Listener

```cpp
void SetListenerParams(const FListenerParams& Params);
```

Stores listener position, velocity, forward, and up vectors for future 3D spatialization.

### Update Loop

```cpp
void Update(float DeltaTime);
```

Must be called each frame. Cleans up finished non-looping voices.

---

## Audio Types

`Engine/Audio/AudioTypes.h`

### EAudioFormat

Identifies PCM audio formats.

| Value | Channels | Bits |
|-------|----------|------|
| `Unknown` | -- | -- |
| `Mono8` | 1 | 8 |
| `Mono16` | 1 | 16 |
| `Stereo8` | 2 | 8 |
| `Stereo16` | 2 | 16 |

```cpp
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
```

### FAudioConfig

Configuration passed to `KAudioManager::Initialize()`.

```cpp
struct FAudioConfig
{
    uint32_t SampleRate   = 44100;
    uint32_t Channels     = 2;
    uint32_t BitsPerSample = 16;
    uint32_t BufferSize   = 4096;
    float MasterVolume    = 1.0f;
    float SoundVolume     = 1.0f;
    float MusicVolume     = 1.0f;
};
```

### FSoundParams

Parameters for a single `PlaySound()` call.

```cpp
struct FSoundParams
{
    std::string Name;
    std::string FilePath;
    float Volume   = 1.0f;
    float Pitch    = 1.0f;
    float Pan      = 0.0f;
    bool  bLooping = false;
    bool  bIs3D    = false;
    bool  bIsMusic = false;
};
```

- `bIsMusic = true` causes the voice volume to be scaled by `MusicVolume` instead of `SoundVolume`.
- `bIs3D = true` flags the sound for 3D spatialization.

### FListenerParams

3D audio listener representation (camera/ear position).

```cpp
struct FListenerParams
{
    float Position[3] = { 0.0f, 0.0f, 0.0f };
    float Velocity[3] = { 0.0f, 0.0f, 0.0f };
    float Forward[3]  = { 0.0f, 0.0f, 1.0f };
    float Up[3]       = { 0.0f, 1.0f, 0.0f };
};
```

### FSound3DParams

Per-sound 3D spatialization parameters.

```cpp
struct FSound3DParams
{
    float Position[3]     = { 0.0f, 0.0f, 0.0f };
    float Velocity[3]     = { 0.0f, 0.0f, 0.0f };
    float MinDistance      = 1.0f;
    float MaxDistance      = 100.0f;
    float RolloffFactor   = 1.0f;
};
```

- `MinDistance` -- distance below which the sound is at full volume
- `MaxDistance` -- distance at which the sound is fully attenuated
- `RolloffFactor` -- controls how quickly volume drops between `MinDistance` and `MaxDistance`

### FSoundState

Runtime playback state for a voice.

```cpp
struct FSoundState
{
    bool  bIsPlaying  = false;
    bool  bIsPaused   = false;
    float CurrentTime = 0.0f;
    float Duration    = 0.0f;
};
```

---

## KAudioSubsystem

`Engine/Audio/AudioSubsystem.h`

Adapter class that wraps `KAudioManager` as an `ISubsystem` for the modular engine architecture.

```cpp
class KAudioSubsystem : public ISubsystem
{
public:
    HRESULT Initialize() override;
    void    Tick(float DeltaTime) override;
    void    Shutdown() override;
    const std::string& GetName() const override;

    KAudioManager*       GetAudioManager();
    const KAudioManager* GetAudioManager() const;
    void                 SetConfig(const FAudioConfig& InConfig);
    const FAudioConfig&  GetConfig() const;
};
```

The subsystem reports `GetName()` as `"AudioSubsystem"` and transitions through `ESubsystemState` values (`Initialized` / `Shutdown`) in its lifecycle methods.

---

## Code Examples

### Basic Initialization

```cpp
FAudioConfig AudioConfig;
AudioConfig.SampleRate    = 44100;
AudioConfig.MasterVolume  = 0.8f;
AudioConfig.SoundVolume   = 1.0f;
AudioConfig.MusicVolume   = 0.7f;

KAudioManager* AudioManager = KAudioManager::GetInstance();
AudioManager->Initialize(AudioConfig);
```

### Loading a Sound

```cpp
AudioManager->LoadSound(L"Assets/Audio/explosion.wav", "explosion");
AudioManager->LoadSound(L"Assets/Audio/ambient_loop.wav", "ambient");
```

### Playing and Controlling Sounds

```cpp
FSoundParams Params;
Params.Volume = 0.8f;
Params.Pitch  = 1.0f;
Params.Pan    = 0.0f;

uint32_t explosionVoice = AudioManager->PlaySound("explosion", Params);

if (AudioManager->IsSoundPlaying(explosionVoice))
{
    AudioManager->PauseSound(explosionVoice);
}

AudioManager->ResumeSound(explosionVoice);
AudioManager->StopSound(explosionVoice);
```

### Looping Background Music

```cpp
FSoundParams MusicParams;
MusicParams.bLooping = true;
MusicParams.bIsMusic = true;
MusicParams.Volume   = 0.5f;

uint32_t musicVoice = AudioManager->PlaySound("ambient", MusicParams);
```

### Adjusting Volume Channels at Runtime

```cpp
AudioManager->SetMasterVolume(0.5f);
AudioManager->SetSoundVolume(0.8f);
AudioManager->SetMusicVolume(0.3f);
```

Note: `SetMusicVolume` immediately updates all currently playing music voices.

### Setting 3D Listener Parameters

```cpp
FListenerParams Listener;
Listener.Position[0] = cameraPos.x;
Listener.Position[1] = cameraPos.y;
Listener.Position[2] = cameraPos.z;
Listener.Forward[0]  = cameraForward.x;
Listener.Forward[1]  = cameraForward.y;
Listener.Forward[2]  = cameraForward.z;
Listener.Up[0]       = cameraUp.x;
Listener.Up[1]       = cameraUp.y;
Listener.Up[2]       = cameraUp.z;

AudioManager->SetListenerParams(Listener);
```

### Playing a 3D Sound

```cpp
FSoundParams Params3D;
Params3D.bIs3D    = true;
Params3D.Volume   = 1.0f;
Params3D.bIsMusic = false;

uint32_t voice3D = AudioManager->PlaySound("explosion", Params3D);
```

### Querying Playback State

```cpp
float duration    = AudioManager->GetSoundDuration(voiceId);
float currentTime = AudioManager->GetSoundCurrentTime(voiceId);
bool  playing     = AudioManager->IsSoundPlaying(voiceId);

LOG_INFO("Playing " + std::to_string(currentTime) + " / " + std::to_string(duration));
```

### Using KAudioSubsystem with the Subsystem Registry

```cpp
auto AudioSubsystem = std::make_unique<KAudioSubsystem>();

FAudioConfig Config;
Config.MasterVolume = 0.8f;
AudioSubsystem->SetConfig(Config);

AudioSubsystem->Initialize();

AudioSubsystem->Tick(deltaTime);

AudioSubsystem->Shutdown();
```

### Full Game Loop Integration

```cpp
void Game::Initialize()
{
    auto* AudioManager = KAudioManager::GetInstance();
    AudioManager->Initialize();

    AudioManager->LoadSound(L"Assets/Audio/bgm.wav", "bgm");
    AudioManager->LoadSound(L"Assets/Audio/jump.wav", "jump");
    AudioManager->LoadSound(L"Assets/Audio/hit.wav", "hit");

    FSoundParams BGMParams;
    BGMParams.bLooping = true;
    BGMParams.bIsMusic = true;
    BGMParams.Volume   = 0.4f;
    AudioManager->PlaySound("bgm", BGMParams);
}

void Game::Update(float DeltaTime)
{
    auto* AudioManager = KAudioManager::GetInstance();
    AudioManager->Update(DeltaTime);

    FListenerParams Listener;
    Listener.Position[0] = Camera.GetPosition().x;
    Listener.Position[1] = Camera.GetPosition().y;
    Listener.Position[2] = Camera.GetPosition().z;
    Listener.Forward[0]  = Camera.GetForward().x;
    Listener.Forward[1]  = Camera.GetForward().y;
    Listener.Forward[2]  = Camera.GetForward().z;
    Listener.Up[0]       = Camera.GetUp().x;
    Listener.Up[1]       = Camera.GetUp().y;
    Listener.Up[2]       = Camera.GetUp().z;
    AudioManager->SetListenerParams(Listener);
}

void Game::Shutdown()
{
    auto* AudioManager = KAudioManager::GetInstance();
    AudioManager->StopAllSounds();
    AudioManager->Shutdown();
}
```

---

## File Reference

| File | Description |
|------|-------------|
| `Engine/Audio/AudioTypes.h` | Enum and struct definitions for audio configuration, sound parameters, 3D parameters |
| `Engine/Audio/Sound.h` | `KSound` class declaration |
| `Engine/Audio/Sound.cpp` | WAV file parsing, in-memory loading, property setters |
| `Engine/Audio/AudioManager.h` | `KAudioManager` singleton declaration |
| `Engine/Audio/AudioManager.cpp` | XAudio2 engine creation, voice management, volume channels, update loop |
| `Engine/Audio/AudioSubsystem.h` | `KAudioSubsystem` `ISubsystem` adapter |
