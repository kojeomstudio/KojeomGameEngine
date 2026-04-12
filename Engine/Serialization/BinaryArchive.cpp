#include "BinaryArchive.h"
#include "../Utils/Logger.h"

void KBinaryArchive::WriteHeader()
{
    uint32 magic = MagicNumber;
    uint32 version = CurrentVersion;
    Write(&magic, sizeof(magic));
    Write(&version, sizeof(version));
    uint32 checksumPlaceholder = 0;
    Write(&checksumPlaceholder, sizeof(checksumPlaceholder));
}

bool KBinaryArchive::ReadAndValidateHeader()
{
    uint32 magic = 0;
    uint32 version = 0;
    uint32 storedChecksum = 0;

    Read(&magic, sizeof(magic));
    Read(&version, sizeof(version));
    Read(&storedChecksum, sizeof(storedChecksum));

    if (magic != MagicNumber)
    {
        LOG_ERROR("Binary archive: invalid magic number");
        bHeaderValid = false;
        return false;
    }

    if (version > CurrentVersion)
    {
        LOG_ERROR("Binary archive: unsupported version " + std::to_string(version));
        bHeaderValid = false;
        return false;
    }

    bHeaderValid = true;
    return true;
}

uint32 KBinaryArchive::ComputeChecksum() const
{
    const size_t headerSize = sizeof(uint32) * 3;
    if (Buffer.size() <= headerSize) return 0;

    uint32 checksum = 0;
    for (size_t i = headerSize; i < Buffer.size(); ++i)
    {
        checksum ^= (static_cast<uint32>(Buffer[i]) << ((i % 4) * 8));
        checksum = (checksum << 1) | (checksum >> 31);
    }
    return checksum;
}

bool KBinaryArchive::Open(const std::wstring& Path)
{
    if (Mode == EMode::Write)
    {
        Stream.open(Path, std::ios::binary | std::ios::out);
        if (!Stream.is_open())
        {
            LOG_ERROR("Failed to open file for writing: " + StringUtils::WideToMultiByte(Path));
            return false;
        }
        Buffer.clear();
        bHasError = false;
        WriteHeader();
    }
    else
    {
        Stream.open(Path, std::ios::binary | std::ios::in);
        if (!Stream.is_open())
        {
            LOG_ERROR("Failed to open file for reading: " + StringUtils::WideToMultiByte(Path));
            return false;
        }

        Stream.seekg(0, std::ios::end);
        size_t fileSize = static_cast<size_t>(Stream.tellg());
        Stream.seekg(0, std::ios::beg);

        if (fileSize > MaxFileSize)
        {
            LOG_ERROR("Binary archive file too large: " + std::to_string(fileSize) + " bytes");
            Stream.close();
            return false;
        }

        Buffer.resize(fileSize);
        Stream.read(reinterpret_cast<char*>(Buffer.data()), fileSize);
        ReadPosition = 0;
        bHasError = false;

        if (!ReadAndValidateHeader())
        {
            LOG_ERROR("Binary archive header validation failed: " + StringUtils::WideToMultiByte(Path));
            Stream.close();
            Buffer.clear();
            return false;
        }
    }

    return true;
}

void KBinaryArchive::Close()
{
    if (Mode == EMode::Write && Stream.is_open() && Buffer.size() >= sizeof(uint32) * 3)
    {
        uint32 checksum = ComputeChecksum();
        memcpy(Buffer.data() + sizeof(uint32) * 2, &checksum, sizeof(checksum));
        Stream.write(reinterpret_cast<const char*>(Buffer.data()), Buffer.size());
    }
    Stream.close();
    Buffer.clear();
    ReadPosition = 0;
    bHeaderValid = false;
    bHasError = false;
}

void KBinaryArchive::Write(const void* Data, size_t Size)
{
    if (Mode != EMode::Write) return;
    const uint8* Bytes = static_cast<const uint8*>(Data);
    Buffer.insert(Buffer.end(), Bytes, Bytes + Size);
}

void KBinaryArchive::Read(void* Data, size_t Size)
{
    if (Mode != EMode::Read) return;
    if (ReadPosition + Size > Buffer.size())
    {
        LOG_ERROR("Binary archive read overflow");
        bHasError = true;
        return;
    }
    memcpy(Data, Buffer.data() + ReadPosition, Size);
    ReadPosition += Size;
}
