#pragma once

#include "../Utils/Common.h"
#include "../Utils/Logger.h"
#include <fstream>
#include <vector>
#include <string>
#include <type_traits>
#include <limits>

class KBinaryArchive
{
public:
    enum class EMode { Read, Write };

    static constexpr uint32 MaxStringSize = 16 * 1024 * 1024;
    static constexpr uint32 MaxFileSize = 512 * 1024 * 1024;

    KBinaryArchive(EMode InMode) : Mode(InMode), ReadPosition(0), bHeaderValid(false), bHasError(false) {}
    ~KBinaryArchive() = default;

    KBinaryArchive(const KBinaryArchive&) = delete;
    KBinaryArchive& operator=(const KBinaryArchive&) = delete;

    bool Open(const std::wstring& Path);
    void Close();
    bool IsOpen() const { return Stream.is_open(); }
    bool IsReading() const { return Mode == EMode::Read; }
    bool IsWriting() const { return Mode == EMode::Write; }
    bool IsHeaderValid() const { return bHeaderValid; }
    bool HasError() const { return bHasError; }

    KBinaryArchive& operator<<(int8 Value) { Write(&Value, sizeof(Value)); return *this; }
    KBinaryArchive& operator<<(uint8 Value) { Write(&Value, sizeof(Value)); return *this; }
    KBinaryArchive& operator<<(int16 Value) { Write(&Value, sizeof(Value)); return *this; }
    KBinaryArchive& operator<<(uint16 Value) { Write(&Value, sizeof(Value)); return *this; }
    KBinaryArchive& operator<<(int32 Value) { Write(&Value, sizeof(Value)); return *this; }
    KBinaryArchive& operator<<(uint32 Value) { Write(&Value, sizeof(Value)); return *this; }
    KBinaryArchive& operator<<(int64 Value) { Write(&Value, sizeof(Value)); return *this; }
    KBinaryArchive& operator<<(uint64 Value) { Write(&Value, sizeof(Value)); return *this; }
    KBinaryArchive& operator<<(float Value) { Write(&Value, sizeof(Value)); return *this; }
    KBinaryArchive& operator<<(double Value) { Write(&Value, sizeof(Value)); return *this; }
    KBinaryArchive& operator<<(bool Value) { Write(&Value, sizeof(Value)); return *this; }

    KBinaryArchive& operator<<(const std::string& Value)
    {
        uint32 Length = static_cast<uint32>(Value.length());
        *this << Length;
        Write(Value.data(), Length);
        return *this;
    }

    KBinaryArchive& operator<<(const std::wstring& Value)
    {
        uint32 Length = static_cast<uint32>(Value.length());
        *this << Length;
        Write(Value.data(), Length * sizeof(wchar_t));
        return *this;
    }

    KBinaryArchive& operator>>(int8& Value) { Read(&Value, sizeof(Value)); return *this; }
    KBinaryArchive& operator>>(uint8& Value) { Read(&Value, sizeof(Value)); return *this; }
    KBinaryArchive& operator>>(int16& Value) { Read(&Value, sizeof(Value)); return *this; }
    KBinaryArchive& operator>>(uint16& Value) { Read(&Value, sizeof(Value)); return *this; }
    KBinaryArchive& operator>>(int32& Value) { Read(&Value, sizeof(Value)); return *this; }
    KBinaryArchive& operator>>(uint32& Value) { Read(&Value, sizeof(Value)); return *this; }
    KBinaryArchive& operator>>(int64& Value) { Read(&Value, sizeof(Value)); return *this; }
    KBinaryArchive& operator>>(uint64& Value) { Read(&Value, sizeof(Value)); return *this; }
    KBinaryArchive& operator>>(float& Value) { Read(&Value, sizeof(Value)); return *this; }
    KBinaryArchive& operator>>(double& Value) { Read(&Value, sizeof(Value)); return *this; }
    KBinaryArchive& operator>>(bool& Value) { Read(&Value, sizeof(Value)); return *this; }

    KBinaryArchive& operator>>(std::string& Value)
    {
        uint32 Length;
        *this >> Length;
        if (Length > MaxStringSize)
        {
            Length = 0;
            Value.clear();
            return *this;
        }
        Value.resize(Length);
        Read(&Value[0], Length);
        return *this;
    }

    KBinaryArchive& operator>>(std::wstring& Value)
    {
        uint32 Length;
        *this >> Length;
        if (Length > MaxStringSize || Length > std::numeric_limits<uint32>::max() / sizeof(wchar_t))
        {
            Length = 0;
            Value.clear();
            return *this;
        }
        Value.resize(Length);
        Read(&Value[0], Length * sizeof(wchar_t));
        return *this;
    }

    void WriteRaw(const void* Data, size_t Size) { Write(Data, Size); }
    void ReadRaw(void* Data, size_t Size) { Read(Data, Size); }

    size_t GetSize() const { return Buffer.size(); }
    const uint8* GetData() const { return Buffer.data(); }

    template<typename T>
    KBinaryArchive& operator<<(const std::vector<T>& Value)
    {
        uint32 Count = static_cast<uint32>(Value.size());
        *this << Count;
        for (const auto& elem : Value)
        {
            *this << elem;
        }
        return *this;
    }

    template<typename T>
    KBinaryArchive& operator>>(std::vector<T>& Value)
    {
        uint32 Count;
        *this >> Count;
        if (Count > 100000)
        {
            bHasError = true;
            LOG_ERROR("Binary archive: vector element count exceeds limit");
            Count = 0;
        }
        Value.resize(Count);
        for (uint32 i = 0; i < Count; ++i)
        {
            *this >> Value[i];
        }
        return *this;
    }

    KBinaryArchive& operator<<(const XMFLOAT3& Value)
    {
        *this << Value.x << Value.y << Value.z;
        return *this;
    }

    KBinaryArchive& operator>>(XMFLOAT3& Value)
    {
        *this >> Value.x >> Value.y >> Value.z;
        return *this;
    }

    KBinaryArchive& operator<<(const XMFLOAT4& Value)
    {
        *this << Value.x << Value.y << Value.z << Value.w;
        return *this;
    }

    KBinaryArchive& operator>>(XMFLOAT4& Value)
    {
        *this >> Value.x >> Value.y >> Value.z >> Value.w;
        return *this;
    }

    KBinaryArchive& operator<<(const XMFLOAT4X4& Value)
    {
        for (int32 i = 0; i < 4; ++i)
        {
            for (int32 j = 0; j < 4; ++j)
            {
                *this << Value.m[i][j];
            }
        }
        return *this;
    }

    KBinaryArchive& operator>>(XMFLOAT4X4& Value)
    {
        for (int32 i = 0; i < 4; ++i)
        {
            for (int32 j = 0; j < 4; ++j)
            {
                *this >> Value.m[i][j];
            }
        }
        return *this;
    }

private:
    void Write(const void* Data, size_t Size);
    void Read(void* Data, size_t Size);

private:
    EMode Mode;
    std::vector<uint8> Buffer;
    size_t ReadPosition;
    std::fstream Stream;
    bool bHeaderValid;
    bool bHasError;

    static constexpr uint32 MagicNumber = 0x4B42494E;
    static constexpr uint32 CurrentVersion = 1;

    uint32 ComputeChecksum() const;
    void WriteHeader();
    bool ReadAndValidateHeader();
};
