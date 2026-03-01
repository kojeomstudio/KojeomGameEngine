#pragma once

#include "../Utils/Common.h"
#include <fstream>
#include <vector>
#include <string>
#include <type_traits>

class KBinaryArchive
{
public:
    enum class EMode { Read, Write };

    KBinaryArchive(EMode InMode) : Mode(InMode), ReadPosition(0) {}
    ~KBinaryArchive() = default;

    KBinaryArchive(const KBinaryArchive&) = delete;
    KBinaryArchive& operator=(const KBinaryArchive&) = delete;

    bool Open(const std::wstring& Path);
    void Close();
    bool IsOpen() const { return Stream.is_open(); }
    bool IsReading() const { return Mode == EMode::Read; }
    bool IsWriting() const { return Mode == EMode::Write; }

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
        Value.resize(Length);
        Read(Value.data(), Length);
        return *this;
    }

    KBinaryArchive& operator>>(std::wstring& Value)
    {
        uint32 Length;
        *this >> Length;
        Value.resize(Length);
        Read(Value.data(), Length * sizeof(wchar_t));
        return *this;
    }

    void WriteRaw(const void* Data, size_t Size) { Write(Data, Size); }
    void ReadRaw(void* Data, size_t Size) { Read(Data, Size); }

    size_t GetSize() const { return Buffer.size(); }
    const uint8* GetData() const { return Buffer.data(); }

private:
    void Write(const void* Data, size_t Size);
    void Read(void* Data, size_t Size);

private:
    EMode Mode;
    std::vector<uint8> Buffer;
    size_t ReadPosition;
    std::fstream Stream;
};
