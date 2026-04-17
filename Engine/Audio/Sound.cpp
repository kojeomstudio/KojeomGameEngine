#include "Sound.h"
#include "../Utils/Common.h"
#include "../Utils/Logger.h"
#include <fstream>
#include <cstring>

static constexpr size_t MaxAudioDataSize = 512 * 1024 * 1024;

#pragma pack(push, 1)
struct FWavHeader
{
    char Riff[4];
    uint32_t FileSize;
    char Wave[4];
    char FmtChunkId[4];
    uint32_t FmtChunkSize;
    uint16_t AudioFormat;
    uint16_t NumChannels;
    uint32_t SampleRate;
    uint32_t ByteRate;
    uint16_t BlockAlign;
    uint16_t BitsPerSample;
};
#pragma pack(pop)

static bool ValidateWavHeader(const FWavHeader& Header)
{
    if (Header.AudioFormat != 1 && Header.AudioFormat != 3)
    {
        LOG_ERROR("WAV: unsupported audio format: " + std::to_string(Header.AudioFormat));
        return false;
    }
    if (Header.NumChannels < 1 || Header.NumChannels > 8)
    {
        LOG_ERROR("WAV: invalid channel count: " + std::to_string(Header.NumChannels));
        return false;
    }
    if (Header.SampleRate < 8000 || Header.SampleRate > 192000)
    {
        LOG_ERROR("WAV: invalid sample rate: " + std::to_string(Header.SampleRate));
        return false;
    }
    if (Header.BitsPerSample != 8 && Header.BitsPerSample != 16 && Header.BitsPerSample != 24 && Header.BitsPerSample != 32)
    {
        LOG_ERROR("WAV: invalid bits per sample: " + std::to_string(Header.BitsPerSample));
        return false;
    }
    uint16_t expectedBlockAlign = static_cast<uint16_t>(Header.NumChannels * Header.BitsPerSample / 8);
    if (Header.BlockAlign != expectedBlockAlign)
    {
        LOG_ERROR("WAV: block align mismatch");
        return false;
    }
    return true;
}

static void FillWaveFormat(std::unique_ptr<uint8_t[]>& WaveFormat, uint32_t Channels, uint32_t SampleRate, uint32_t ByteRate, uint32_t BlockAlign, uint32_t BitsPerSample)
{
    WaveFormat = std::make_unique<uint8_t[]>(sizeof(WAVEFORMATEX));
    WAVEFORMATEX* pFormat = reinterpret_cast<WAVEFORMATEX*>(WaveFormat.get());
    pFormat->wFormatTag = WAVE_FORMAT_PCM;
    pFormat->nChannels = static_cast<WORD>(Channels);
    pFormat->nSamplesPerSec = SampleRate;
    pFormat->nAvgBytesPerSec = ByteRate;
    pFormat->nBlockAlign = static_cast<WORD>(BlockAlign);
    pFormat->wBitsPerSample = static_cast<WORD>(BitsPerSample);
    pFormat->cbSize = 0;
}

KSound::KSound()
    : SampleRate(44100)
    , Channels(2)
    , BitsPerSample(16)
    , ByteRate(0)
    , BlockAlign(0)
    , DataSize(0)
    , Volume(1.0f)
    , Pitch(1.0f)
    , Pan(0.0f)
    , Duration(0.0f)
    , bIsLooping(false)
    , bIs3D(false)
{
}

KSound::~KSound()
{
    AudioData.clear();
    WaveFormat.reset();
}

bool KSound::LoadFromWavFile(const std::wstring& FilePath)
{
    if (PathUtils::ContainsTraversal(FilePath))
    {
        LOG_ERROR("Sound: path contains traversal patterns");
        return false;
    }

    if (!PathUtils::IsPathSafe(FilePath, L"."))
    {
        LOG_WARNING("Sound: path is outside allowed directory");
        return false;
    }

    std::ifstream File(FilePath, std::ios::binary);
    if (!File.is_open())
    {
        LOG_ERROR("Failed to open wav file: " + StringUtils::WideToMultiByte(FilePath));
        return false;
    }

    File.seekg(0, std::ios::end);
    auto fileSize = File.tellg();
    File.seekg(0, std::ios::beg);

    if (fileSize > static_cast<std::streamoff>(MaxAudioDataSize))
    {
        LOG_ERROR("WAV file too large: " + std::to_string(fileSize) + " bytes");
        return false;
    }

    if (fileSize < static_cast<std::streamoff>(sizeof(FWavHeader)))
    {
        LOG_ERROR("WAV file too small: " + std::to_string(fileSize) + " bytes");
        return false;
    }

    FWavHeader Header;
    File.read(reinterpret_cast<char*>(&Header), sizeof(FWavHeader));

    if (std::strncmp(Header.Riff, "RIFF", 4) != 0 || std::strncmp(Header.Wave, "WAVE", 4) != 0)
    {
        LOG_ERROR("Invalid WAV file format");
        return false;
    }

    if (!ValidateWavHeader(Header))
    {
        return false;
    }

    char ChunkId[4];
    uint32_t ChunkSize;

    while (File.read(ChunkId, 4))
    {
        File.read(reinterpret_cast<char*>(&ChunkSize), 4);

        if (std::strncmp(ChunkId, "data", 4) == 0)
        {
            if (ChunkSize > MaxAudioDataSize)
            {
                LOG_ERROR("WAV data chunk too large: " + std::to_string(ChunkSize));
                return false;
            }
            DataSize = ChunkSize;
            AudioData.resize(DataSize);
            File.read(reinterpret_cast<char*>(AudioData.data()), DataSize);
            break;
        }
        else
        {
            File.seekg(ChunkSize, std::ios::cur);
        }
    }

    File.close();

    Channels = Header.NumChannels;
    SampleRate = Header.SampleRate;
    BitsPerSample = Header.BitsPerSample;
    ByteRate = Header.ByteRate;
    BlockAlign = Header.BlockAlign;

    FillWaveFormat(WaveFormat, Channels, SampleRate, ByteRate, BlockAlign, BitsPerSample);

    CalculateDuration();

    LOG_INFO("Loaded WAV file: " + StringUtils::WideToMultiByte(FilePath) + 
             " (" + std::to_string(Channels) + " ch, " + 
             std::to_string(SampleRate) + " Hz, " +
             std::to_string(BitsPerSample) + " bits, " +
             std::to_string(Duration) + " sec)");

    return true;
}

bool KSound::LoadFromMemory(const uint8_t* Data, size_t Size, uint32_t InSampleRate, uint32_t InChannels, uint32_t InBitsPerSample)
{
    if (!Data || Size == 0)
    {
        return false;
    }

    if (Size > MaxAudioDataSize)
    {
        LOG_ERROR("Audio data too large for memory load: " + std::to_string(Size));
        return false;
    }

    SampleRate = InSampleRate;
    Channels = InChannels;
    BitsPerSample = InBitsPerSample;
    DataSize = static_cast<uint32_t>(Size);

    BlockAlign = static_cast<uint32_t>(Channels * BitsPerSample / 8);
    ByteRate = SampleRate * BlockAlign;

    AudioData.resize(Size);
    std::memcpy(AudioData.data(), Data, Size);

    FillWaveFormat(WaveFormat, Channels, SampleRate, ByteRate, BlockAlign, BitsPerSample);

    CalculateDuration();

    return true;
}

void KSound::SetVolume(float InVolume)
{
    Volume = std::max(0.0f, std::min(1.0f, InVolume));
}

void KSound::SetPitch(float InPitch)
{
    Pitch = std::max(0.5f, std::min(2.0f, InPitch));
}

void KSound::SetPan(float InPan)
{
    Pan = std::max(-1.0f, std::min(1.0f, InPan));
}

void KSound::CalculateDuration()
{
    if (ByteRate > 0)
    {
        Duration = static_cast<float>(DataSize) / static_cast<float>(ByteRate);
    }
    else
    {
        Duration = 0.0f;
    }
}
