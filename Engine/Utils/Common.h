#pragma once

#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

#define NOMINMAX

#include <windows.h>

#include <d3d11_1.h>
#include <directxcolors.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <type_traits>
#include <iostream>
#include <crtdbg.h>

using namespace DirectX;

using int8   = int8_t;
using int16  = int16_t;
using int32  = int32_t;
using int64  = int64_t;
using uint8  = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

#define SAFE_RELEASE(p) do { if((p)) { (p)->Release(); (p) = nullptr; } } while(0)
#define SAFE_DELETE(p) do { if((p)) { delete (p); (p) = nullptr; } } while(0)
#define SAFE_DELETE_ARRAY(p) do { if((p)) { delete[] (p); (p) = nullptr; } } while(0)

#define CHECK_HRESULT(hr) do { if(FAILED(hr)) return (hr); } while(0)
#define LOG_ERROR(msg) KLogger::Error(msg)
#define LOG_INFO(msg) KLogger::Info(msg)
#define LOG_WARNING(msg) KLogger::Warning(msg)

template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

class KEngine;
class KGraphicsDevice;
class KRenderer;
class KCamera;
class KLogger;

namespace StringUtils
{
    inline std::string WideToMultiByte(const std::wstring& WideStr)
    {
        if (WideStr.empty()) return std::string();
        
        int SizeNeeded = WideCharToMultiByte(CP_UTF8, 0, &WideStr[0], (int)WideStr.size(), NULL, 0, NULL, NULL);
        std::string StrTo(SizeNeeded, 0);
        WideCharToMultiByte(CP_UTF8, 0, &WideStr[0], (int)WideStr.size(), &StrTo[0], SizeNeeded, NULL, NULL);
        return StrTo;
    }

    inline std::wstring MultiByteToWide(const std::string& MultiStr)
    {
        if (MultiStr.empty()) return std::wstring();
        
        int SizeNeeded = MultiByteToWideChar(CP_UTF8, 0, &MultiStr[0], (int)MultiStr.size(), NULL, 0);
        std::wstring WStrTo(SizeNeeded, 0);
        MultiByteToWideChar(CP_UTF8, 0, &MultiStr[0], (int)MultiStr.size(), &WStrTo[0], SizeNeeded);
        return WStrTo;
    }
}

namespace PathUtils
{
    inline bool ContainsPercentEncoding(const std::wstring& Path)
    {
        for (size_t i = 0; i + 2 < Path.size(); ++i)
        {
            if (Path[i] == L'%' && isxdigit(Path[i + 1]) && isxdigit(Path[i + 2]))
            {
                return true;
            }
        }
        return false;
    }

    inline bool ContainsTraversal(const std::wstring& Path)
    {
        if (Path.empty()) return true;

        if (ContainsPercentEncoding(Path)) return true;

        std::wstring normalized = Path;
        for (auto& c : normalized)
        {
            if (c == L'/') c = L'\\';
        }

        if (normalized.find(L"..\\") != std::wstring::npos)
        {
            return true;
        }

        if (normalized == L"..")
        {
            return true;
        }

        if (normalized.size() >= 2 && normalized[normalized.size() - 1] == L'.' &&
            normalized[normalized.size() - 2] == L'.')
        {
            size_t pos = normalized.size() - 2;
            if (pos == 0 || normalized[pos - 1] == L'\\')
            {
                return true;
            }
        }

        if (normalized.size() >= 2 && normalized[0] == L'\\' && normalized[1] == L'\\')
        {
            return true;
        }

        if (normalized.size() >= 4 && normalized.substr(0, 4) == L"\\\\?\\")
        {
            return true;
        }

        if (normalized.find(L"::$") != std::wstring::npos)
        {
            return true;
        }

        if (!normalized.empty() && normalized[0] == L'\\')
        {
            return true;
        }

        if (normalized.size() >= 3 && normalized[1] == L':' && normalized[2] == L'\\')
        {
            wchar_t drive = towupper(normalized[0]);
            if (drive >= L'A' && drive <= L'Z')
            {
                return true;
            }
        }

        return false;
    }

    inline bool IsPathSafe(const std::wstring& Path, const std::wstring& BaseDir)
    {
        if (ContainsTraversal(Path)) return false;

        if (BaseDir.empty()) return true;

        std::wstring fullPath(_MAX_PATH, L'\0');
        if (!_wfullpath(&fullPath[0], Path.c_str(), fullPath.size()))
        {
            fullPath.resize(32768, L'\0');
            if (!_wfullpath(&fullPath[0], Path.c_str(), fullPath.size())) return false;
        }
        fullPath.resize(wcslen(fullPath.c_str()));

        std::wstring resolvedBase(_MAX_PATH, L'\0');
        if (!_wfullpath(&resolvedBase[0], BaseDir.c_str(), resolvedBase.size()))
        {
            resolvedBase.resize(32768, L'\0');
            if (!_wfullpath(&resolvedBase[0], BaseDir.c_str(), resolvedBase.size())) return false;
        }
        resolvedBase.resize(wcslen(resolvedBase.c_str()));

        if (fullPath.compare(0, resolvedBase.size(), resolvedBase) != 0) return false;
        if (resolvedBase.size() > 0 && fullPath.size() > resolvedBase.size() &&
            fullPath[resolvedBase.size()] != L'\\') return false;

        return true;
    }
}

namespace EngineConstants
{
    constexpr UINT32 DEFAULT_WINDOW_WIDTH = 1024;
    constexpr UINT32 DEFAULT_WINDOW_HEIGHT = 768;
    constexpr float DEFAULT_FOV = XM_PIDIV4;
    constexpr float DEFAULT_NEAR_PLANE = 0.1f;
    constexpr float DEFAULT_FAR_PLANE = 1000.0f;
} 
