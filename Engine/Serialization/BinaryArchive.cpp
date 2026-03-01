#include "BinaryArchive.h"
#include "../Utils/Logger.h"

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

        Buffer.resize(fileSize);
        Stream.read(reinterpret_cast<char*>(Buffer.data()), fileSize);
        ReadPosition = 0;
    }

    return true;
}

void KBinaryArchive::Close()
{
    if (Mode == EMode::Write && Stream.is_open())
    {
        Stream.write(reinterpret_cast<const char*>(Buffer.data()), Buffer.size());
    }
    Stream.close();
    Buffer.clear();
    ReadPosition = 0;
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
        return;
    }
    memcpy(Data, Buffer.data() + ReadPosition, Size);
    ReadPosition += Size;
}
