#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdint>

namespace Kojeom
{
class FileSystem
{
public:
    static bool FileExists(const std::string& path)
    {
        std::ifstream f(path);
        return f.good();
    }

    static std::string ReadTextFile(const std::string& path)
    {
        std::ifstream f(path);
        if (!f.is_open()) return {};
        std::stringstream ss;
        ss << f.rdbuf();
        return ss.str();
    }

    static std::vector<uint8_t> ReadBinaryFile(const std::string& path)
    {
        std::ifstream f(path, std::ios::binary | std::ios::ate);
        if (!f.is_open()) return {};
        auto size = f.tellg();
        f.seekg(0);
        std::vector<uint8_t> data(static_cast<size_t>(size));
        f.read(reinterpret_cast<char*>(data.data()), size);
        return data;
    }

    static bool WriteTextFile(const std::string& path, const std::string& content)
    {
        std::ofstream f(path);
        if (!f.is_open()) return false;
        f << content;
        return true;
    }

    static bool WriteBinaryFile(const std::string& path, const std::vector<uint8_t>& data)
    {
        std::ofstream f(path, std::ios::binary);
        if (!f.is_open()) return false;
        f.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
        return true;
    }

    static std::string GetDirectory(const std::string& path)
    {
        auto pos = path.find_last_of("/\\");
        return (pos != std::string::npos) ? path.substr(0, pos + 1) : "./";
    }

    static std::string GetFileName(const std::string& path)
    {
        auto pos = path.find_last_of("/\\");
        return (pos != std::string::npos) ? path.substr(pos + 1) : path;
    }

    static std::string GetExtension(const std::string& path)
    {
        auto pos = path.find_last_of('.');
        return (pos != std::string::npos) ? path.substr(pos) : "";
    }

    static std::string NormalizePath(const std::string& path)
    {
        std::string result = path;
        for (auto& c : result)
        {
            if (c == '\\') c = '/';
        }
        while (result.find("//") != std::string::npos)
        {
            result.replace(result.find("//"), 2, "/");
        }
        return result;
    }

    static bool ValidatePath(const std::string& path)
    {
        if (path.find("..") != std::string::npos) return false;
        if (path.empty()) return false;
        return true;
    }
};
}
