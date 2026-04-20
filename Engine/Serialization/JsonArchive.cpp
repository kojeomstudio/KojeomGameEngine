#include "JsonArchive.h"
#include "../Utils/Logger.h"
#include <fstream>
#include <sstream>

bool KJsonArchive::LoadFromFile(const std::wstring& Path)
{
    if (PathUtils::ContainsTraversal(Path) || !PathUtils::IsPathSafe(Path, L"."))
    {
        LOG_ERROR("JSON archive: path contains traversal patterns");
        return false;
    }

    std::ifstream file(Path, std::ios::binary);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to open JSON file: " + StringUtils::WideToMultiByte(Path));
        return false;
    }

    file.seekg(0, std::ios::end);
    auto fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (fileSize > 100 * 1024 * 1024)
    {
        LOG_ERROR("JSON file too large: " + std::to_string(fileSize) + " bytes");
        file.close();
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    return DeserializeFromString(buffer.str());
}

bool KJsonArchive::SaveToFile(const std::wstring& Path)
{
    if (PathUtils::ContainsTraversal(Path) || !PathUtils::IsPathSafe(Path, L"."))
    {
        LOG_ERROR("JSON archive: path contains traversal patterns");
        return false;
    }

    std::ofstream file(Path);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to create JSON file: " + StringUtils::WideToMultiByte(Path));
        return false;
    }

    file << SerializeToString();
    file.close();

    return true;
}

std::string KJsonArchive::SerializeToString() const
{
    if (!Root)
    {
        return "{}";
    }

    std::ostringstream stream;
    SerializeValue(stream, Root);
    return stream.str();
}

bool KJsonArchive::DeserializeFromString(const std::string& JsonString)
{
    size_t pos = 0;
    SkipWhitespace(JsonString, pos);

    if (pos >= JsonString.size())
    {
        Root = CreateObject();
        return true;
    }

    auto value = ParseValue(JsonString, pos);
    if (value && value->IsObject())
    {
        Root = value->AsObject();
        return true;
    }

    Root = CreateObject();
    return false;
}

std::string KJsonArchive::EscapeString(const std::string& Str) const
{
    std::string result;
    result.reserve(Str.size());

    for (char c : Str)
    {
        switch (c)
        {
        case '"': result += "\\\""; break;
        case '\\': result += "\\\\"; break;
        case '\b': result += "\\b"; break;
        case '\f': result += "\\f"; break;
        case '\n': result += "\\n"; break;
        case '\r': result += "\\r"; break;
        case '\t': result += "\\t"; break;
        default:
            if (static_cast<unsigned char>(c) < 0x20)
            {
                char buf[8];
                sprintf_s(buf, "\\u%04x", static_cast<unsigned char>(c));
                result += buf;
            }
            else
            {
                result += c;
            }
        }
    }

    return result;
}

std::string KJsonArchive::UnescapeString(const std::string& Str) const
{
    std::string result;
    result.reserve(Str.size());

    for (size_t i = 0; i < Str.size(); ++i)
    {
        if (Str[i] == '\\' && i + 1 < Str.size())
        {
            switch (Str[i + 1])
            {
            case '"': result += '"'; ++i; break;
            case '\\': result += '\\'; ++i; break;
            case 'b': result += '\b'; ++i; break;
            case 'f': result += '\f'; ++i; break;
            case 'n': result += '\n'; ++i; break;
            case 'r': result += '\r'; ++i; break;
            case 't': result += '\t'; ++i; break;
            case 'u':
                if (i + 5 < Str.size())
                {
                    unsigned int codepoint = 0;
                    for (int j = 0; j < 4; ++j)
                    {
                        codepoint <<= 4;
                        char hex = Str[i + 2 + j];
                        if (hex >= '0' && hex <= '9') codepoint |= hex - '0';
                        else if (hex >= 'a' && hex <= 'f') codepoint |= hex - 'a' + 10;
                        else if (hex >= 'A' && hex <= 'F') codepoint |= hex - 'A' + 10;
                    }
                    if (codepoint < 0x80)
                    {
                        result += static_cast<char>(codepoint);
                    }
                    else if (codepoint < 0x800)
                    {
                        result += static_cast<char>(0xC0 | (codepoint >> 6));
                        result += static_cast<char>(0x80 | (codepoint & 0x3F));
                    }
                    else
                    {
                        result += static_cast<char>(0xE0 | (codepoint >> 12));
                        result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                        result += static_cast<char>(0x80 | (codepoint & 0x3F));
                    }
                    i += 5;
                }
                break;
            default:
                result += Str[i];
            }
        }
        else
        {
            result += Str[i];
        }
    }

    return result;
}

void KJsonArchive::SerializeValue(std::ostringstream& Stream, const JsonValuePtr& Value) const
{
    if (!Value)
    {
        Stream << "null";
        return;
    }

    switch (Value->GetType())
    {
    case EJsonType::Null:
        Stream << "null";
        break;
    case EJsonType::Bool:
        Stream << (Value->AsBool() ? "true" : "false");
        break;
    case EJsonType::Number:
        Stream << Value->AsNumber();
        break;
    case EJsonType::String:
        Stream << "\"" << EscapeString(Value->AsString()) << "\"";
        break;
    case EJsonType::Array:
        {
            Stream << "[";
            auto arr = Value->AsArray();
            const auto& values = arr->GetValues();
            for (size_t i = 0; i < values.size(); ++i)
            {
                if (i > 0) Stream << ",";
                SerializeValue(Stream, values[i]);
            }
            Stream << "]";
        }
        break;
    case EJsonType::Object:
        {
            Stream << "{";
            auto obj = Value->AsObject();
            const auto& values = obj->GetValues();
            bool first = true;
            for (const auto& pair : values)
            {
                if (!first) Stream << ",";
                first = false;
                Stream << "\"" << EscapeString(pair.first) << "\":";
                SerializeValue(Stream, pair.second);
            }
            Stream << "}";
        }
        break;
    default:
        Stream << "null";
    }
}

void KJsonArchive::SkipWhitespace(const std::string& Str, size_t& Pos) const
{
    while (Pos < Str.size() && (Str[Pos] == ' ' || Str[Pos] == '\t' || Str[Pos] == '\n' || Str[Pos] == '\r'))
    {
        ++Pos;
    }
}

JsonValuePtr KJsonArchive::ParseValue(const std::string& Str, size_t& Pos, int32 Depth) const
{
    SkipWhitespace(Str, Pos);

    if (Pos >= Str.size())
    {
        return std::make_shared<KJsonNull>();
    }

    if (Depth > MaxNestingDepth)
    {
        LOG_ERROR("JSON parser: maximum nesting depth exceeded");
        return std::make_shared<KJsonNull>();
    }

    char c = Str[Pos];

    if (c == '{')
    {
        return ParseObject(Str, Pos, Depth + 1);
    }
    else if (c == '[')
    {
        return ParseArray(Str, Pos, Depth + 1);
    }
    else if (c == '"')
    {
        return std::make_shared<KJsonString>(ParseString(Str, Pos));
    }
    else if (c == 't' || c == 'f')
    {
        return std::make_shared<KJsonBool>(ParseBool(Str, Pos));
    }
    else if (c == 'n')
    {
        if (Pos + 4 <= Str.size() && Str.compare(Pos, 4, "null") == 0)
        {
            Pos += 4;
            return std::make_shared<KJsonNull>();
        }
        return std::make_shared<KJsonNull>();
    }
    else if ((c >= '0' && c <= '9') || c == '-' || c == '+')
    {
        return std::make_shared<KJsonNumber>(ParseNumber(Str, Pos));
    }

    return std::make_shared<KJsonNull>();
}

JsonObjectPtr KJsonArchive::ParseObject(const std::string& Str, size_t& Pos, int32 Depth) const
{
    auto obj = CreateObject();

    ++Pos;
    SkipWhitespace(Str, Pos);

    if (Pos < Str.size() && Str[Pos] == '}')
    {
        ++Pos;
        return obj;
    }

    size_t propertyCount = 0;
    while (Pos < Str.size())
    {
        SkipWhitespace(Str, Pos);

        if (Pos >= Str.size()) break;

        if (Str[Pos] != '"')
        {
            break;
        }

        std::string key = ParseString(Str, Pos);

        SkipWhitespace(Str, Pos);
        if (Pos >= Str.size() || Str[Pos] != ':')
        {
            break;
        }
        ++Pos;

        auto value = ParseValue(Str, Pos, Depth);
        obj->Set(key, value);

        ++propertyCount;
        if (propertyCount > MaxObjectProperties)
        {
            LOG_ERROR("JSON parser: object property count exceeds limit");
            break;
        }

        SkipWhitespace(Str, Pos);
        if (Pos >= Str.size())
        {
            break;
        }

        if (Str[Pos] == '}')
        {
            ++Pos;
            break;
        }

        if (Str[Pos] == ',')
        {
            ++Pos;
        }
    }

    return obj;
}

JsonArrayPtr KJsonArchive::ParseArray(const std::string& Str, size_t& Pos, int32 Depth) const
{
    auto arr = CreateArray();

    ++Pos;
    SkipWhitespace(Str, Pos);

    if (Pos < Str.size() && Str[Pos] == ']')
    {
        ++Pos;
        return arr;
    }

    size_t elementCount = 0;
    while (Pos < Str.size())
    {
        auto value = ParseValue(Str, Pos, Depth);
        arr->Add(value);

        ++elementCount;
        if (elementCount > MaxArrayElements)
        {
            LOG_ERROR("JSON parser: array element count exceeds limit");
            break;
        }

        SkipWhitespace(Str, Pos);
        if (Pos >= Str.size())
        {
            break;
        }

        if (Str[Pos] == ']')
        {
            ++Pos;
            break;
        }

        if (Str[Pos] == ',')
        {
            ++Pos;
        }
    }

    return arr;
}

std::string KJsonArchive::ParseString(const std::string& Str, size_t& Pos) const
{
    ++Pos;

    std::string result;
    while (Pos < Str.size() && Str[Pos] != '"')
    {
        if (Str[Pos] == '\\' && Pos + 1 < Str.size())
        {
            ++Pos;
            switch (Str[Pos])
            {
            case '"': result += '"'; break;
            case '\\': result += '\\'; break;
            case 'b': result += '\b'; break;
            case 'f': result += '\f'; break;
            case 'n': result += '\n'; break;
            case 'r': result += '\r'; break;
            case 't': result += '\t'; break;
            case 'u':
                if (Pos + 4 < Str.size())
                {
                    unsigned int codepoint = 0;
                    for (int i = 0; i < 4; ++i)
                    {
                        codepoint <<= 4;
                        char hex = Str[Pos + 1 + i];
                        if (hex >= '0' && hex <= '9') codepoint |= hex - '0';
                        else if (hex >= 'a' && hex <= 'f') codepoint |= hex - 'a' + 10;
                        else if (hex >= 'A' && hex <= 'F') codepoint |= hex - 'A' + 10;
                    }
                    if (codepoint < 0x80)
                    {
                        result += static_cast<char>(codepoint);
                    }
                    else if (codepoint < 0x800)
                    {
                        result += static_cast<char>(0xC0 | (codepoint >> 6));
                        result += static_cast<char>(0x80 | (codepoint & 0x3F));
                    }
                    else
                    {
                        result += static_cast<char>(0xE0 | (codepoint >> 12));
                        result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                        result += static_cast<char>(0x80 | (codepoint & 0x3F));
                    }
                    Pos += 4;
                }
                break;
            default:
                result += Str[Pos];
            }
        }
        else
        {
            result += Str[Pos];
        }
        ++Pos;
    }

    if (Pos < Str.size() && Str[Pos] == '"')
    {
        ++Pos;
    }

    return result;
}

double KJsonArchive::ParseNumber(const std::string& Str, size_t& Pos) const
{
    size_t start = Pos;

    if (Pos >= Str.size()) return 0.0;

    if (Str[Pos] == '-')
    {
        ++Pos;
    }

    if (Pos >= Str.size()) return 0.0;

    while (Pos < Str.size() && Str[Pos] >= '0' && Str[Pos] <= '9')
    {
        ++Pos;
    }

    if (Pos < Str.size() && Str[Pos] == '.')
    {
        ++Pos;
        while (Pos < Str.size() && Str[Pos] >= '0' && Str[Pos] <= '9')
        {
            ++Pos;
        }
    }

    if (Pos < Str.size() && (Str[Pos] == 'e' || Str[Pos] == 'E'))
    {
        ++Pos;
        if (Pos < Str.size() && (Str[Pos] == '+' || Str[Pos] == '-'))
        {
            ++Pos;
        }
        while (Pos < Str.size() && Str[Pos] >= '0' && Str[Pos] <= '9')
        {
            ++Pos;
        }
    }

    try
    {
        return std::stod(Str.substr(start, Pos - start));
    }
    catch (const std::exception&)
    {
        return 0.0;
    }
}

bool KJsonArchive::ParseBool(const std::string& Str, size_t& Pos) const
{
    if (Pos + 4 <= Str.size() && Str.substr(Pos, 4) == "true")
    {
        Pos += 4;
        return true;
    }
    else if (Pos + 5 <= Str.size() && Str.substr(Pos, 5) == "false")
    {
        Pos += 5;
        return false;
    }
    return false;
}
