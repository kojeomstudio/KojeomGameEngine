#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <cerrno>
#include <algorithm>
#include <sys/stat.h>

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
        if (!ValidatePath(path)) return false;
        std::ofstream f(path);
        if (!f.is_open()) return false;
        f << content;
        return true;
    }

    static bool WriteBinaryFile(const std::string& path, const std::vector<uint8_t>& data)
    {
        if (!ValidatePath(path)) return false;
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
        if (path.empty()) return false;
        if (path.find("..") != std::string::npos) return false;
        if (path.find('\0') != std::string::npos) return false;
        if (path.find("%2e%2e") != std::string::npos) return false;
        if (path.find("%2E%2E") != std::string::npos) return false;
        if (path.find("%2e%2E") != std::string::npos) return false;
        if (path.find("%2E%2e") != std::string::npos) return false;
        if (path.find("%2f") != std::string::npos) return false;
        if (path.find("%2F") != std::string::npos) return false;
        if (path.find("%5c") != std::string::npos) return false;
        if (path.find("%5C") != std::string::npos) return false;
#ifdef _WIN32
        if (path.size() >= 2 && path[1] == ':') return false;
#else
        if (!path.empty() && path[0] == '/') return false;
#endif
        return true;
    }

    static std::string SanitizePath(const std::string& path)
    {
        std::string result = NormalizePath(path);
        if (result.find("..") != std::string::npos)
        {
            std::vector<std::string> parts;
            std::istringstream iss(result);
            std::string part;
            while (std::getline(iss, part, '/'))
            {
                if (part == ".." && !parts.empty())
                    parts.pop_back();
                else if (part != "." && !part.empty())
                    parts.push_back(part);
            }
            result.clear();
            for (size_t i = 0; i < parts.size(); ++i)
            {
                if (i > 0) result += "/";
                result += parts[i];
            }
        }
        return result;
    }

    static bool CreateDirectory(const std::string& path)
    {
        if (path.empty()) return false;
        std::string normalized = NormalizePath(path);
        std::vector<std::string> parts;
        std::istringstream iss(normalized);
        std::string part;
        while (std::getline(iss, part, '/'))
        {
            if (!part.empty())
                parts.push_back(part);
        }
        std::string current;
        for (const auto& p : parts)
        {
            current += p + "/";
            struct stat st;
            if (stat(current.c_str(), &st) != 0)
            {
                if (mkdir(current.c_str(), 0755) != 0 && errno != EEXIST)
                    return false;
            }
        }
        return true;
    }
};
}
